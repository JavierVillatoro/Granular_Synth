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
    // 1. QUITAMOS la variable audioReceived del bucle para que sea persistente
    while (!threadShouldExit())
    {
        // Esperamos una conexión (esto bloquea el hilo hasta que llega algo)
        juce::StreamingSocket* clientConnection = listenerSocket->waitForNextConnection();

        if (clientConnection != nullptr && !threadShouldExit())
        {
            DBG("¡Conexión entrante detectada!");

            // --- TU LÓGICA DE CABECERAS (Igual) ---
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

            // --- TU LÓGICA DE PROCESAMIENTO (Modificada levemente) ---
            if (contentLength > 0 && headerString.contains("POST"))
            {
                DBG("Descargando audio de " + juce::String(contentLength) + " bytes...");

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

                // Usamos callAsync para cargar el audio y apagar el botón REC en la UI
                juce::MessageManager::callAsync([this, filePath] {
                    processorRef.loadFile(filePath, targetLayer);

                    // Esto apaga el botón rojo automáticamente al terminar
                    auto* p = processorRef.apvts.getParameter("L" + juce::String(targetLayer) + "_REC");
                    if (p != nullptr) p->setValueNotifyingHost(0.0f);
                    });

                // ¡IMPORTANTE! No ponemos audioReceived = true. 
                // El bucle volverá arriba y esperará el siguiente mensaje o que apagues el REC.
            }

            // --- RESPUESTA HTTP (Obligatoria para que el móvil no se quede esperando) ---
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
