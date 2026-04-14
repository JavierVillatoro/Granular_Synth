/*
  ==============================================================================

    TcpReceiver.cpp
    Created: 14 Apr 2026 2:14:46am
    Author:  franc

  ==============================================================================
*/

#include "TcpReceiver.h"
#include "PluginProcessor.h"

TcpReceiver::TcpReceiver(Granular_SynthAudioProcessor& p)
    : Thread("TCP_Receiver_Thread"), processorRef(p)
{
}

TcpReceiver::~TcpReceiver()
{
    stopListening();
}

void TcpReceiver::startListening(int layerToUpdate)
{
    targetLayer = layerToUpdate;

    listenerSocket = std::make_unique<juce::StreamingSocket>();

    if (listenerSocket->createListener(8080)) {
        DBG("Servidor TCP Iniciado en puerto 8080. Esperando audio para la capa " + juce::String(targetLayer));
        startThread();
    }
    else {
        DBG("ERROR: No se pudo abrir el puerto 8080 (quizás ya está en uso).");
    }
}

void TcpReceiver::stopListening()
{
    signalThreadShouldExit();
    if (listenerSocket != nullptr) {
        listenerSocket->close();
    }
    stopThread(2000);
}

void TcpReceiver::run()
{
    bool audioReceived = false; // <-- LA CLAVE: No nos iremos hasta que esto sea true

    while (!threadShouldExit() && !audioReceived)
    {
        juce::StreamingSocket* clientConnection = listenerSocket->waitForNextConnection();

        if (clientConnection != nullptr && !threadShouldExit())
        {
            DBG("¡Conexión entrante detectada!");

            // 1. Leer las cabeceras HTTP
            juce::String headerString;
            char charBuf[1];
            while (clientConnection->read(charBuf, 1, true) == 1) {
                headerString += juce::String::charToString(charBuf[0]);
                if (headerString.endsWith("\r\n\r\n")) break;
            }

            int contentLength = 0;
            int idx = headerString.indexOfIgnoreCase("Content-Length:");
            if (idx >= 0) {
                contentLength = headerString.substring(idx + 15).trimStart().getIntValue();
            }

            // 2. Si es un POST y tiene peso, ¡es nuestro audio!
            if (contentLength > 0 && headerString.contains("POST"))
            {
                DBG("Descargando archivo WAV de " + juce::String(contentLength) + " bytes...");

                juce::MemoryBlock audioData;
                audioData.setSize((size_t)contentLength);
                int totalRead = 0;
                while (totalRead < contentLength) {
                    int readNow = clientConnection->read((char*)audioData.getData() + totalRead, contentLength - totalRead, true);
                    if (readNow <= 0) break;
                    totalRead += readNow;
                }

                juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("granular_wifi_layer_" + juce::String(targetLayer) + ".wav");
                tempFile.replaceWithData(audioData.getData(), audioData.getSize());

                juce::String filePath = tempFile.getFullPathName();
                DBG("Archivo guardado con éxito en: " + filePath);

                juce::MessageManager::callAsync([this, filePath] {
                    processorRef.loadFile(filePath, targetLayer);

                    auto* p = processorRef.apvts.getParameter("L" + juce::String(targetLayer) + "_REC");
                    if (p != nullptr) p->setValueNotifyingHost(0.0f);
                    });

                audioReceived = true; // ¡AHORA SÍ SALIMOS DEL BUCLE MAESTRO!
            }
            else
            {
                // Si es un ping de seguridad del navegador (OPTIONS), lo ignoramos y seguimos escuchando
                DBG("Petición de seguridad del navegador ignorada. Esperando el audio real...");
            }

            // 3. Responder SIEMPRE con las cabeceras mágicas para calmar al navegador
            juce::String response = "HTTP/1.1 200 OK\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                "Access-Control-Allow-Headers: *\r\n"
                "Connection: close\r\n\r\nOK";
            clientConnection->write(response.toRawUTF8(), response.getNumBytesAsUTF8());

            delete clientConnection;
        }
    }
    DBG("Servidor TCP Apagado.");
}
