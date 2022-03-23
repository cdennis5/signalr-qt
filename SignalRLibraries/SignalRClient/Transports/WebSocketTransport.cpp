#include "WebSocketTransport.h"
#include "Helper/Helper.h"
#include "SignalException.h"
#include "Connection_p.h"

namespace P3 { namespace SignalR { namespace Client {

WebSocketTransport::WebSocketTransport() :
    HttpBasedTransport(),
    _webSocket(0),
    _started(false),
    _shakingHands(false),
    _handShakeCompleted(false)
{}

void WebSocketTransport::start(QString)
{
    _handShakeCompleted = false;
     _connection->updateLastRetryTime();

    if(_webSocket)
    {
        _webSocket->deleteLater();
        _webSocket = 0;
    }
    if(_webSocket == 0)
    {
        _webSocket = new QWebSocket();
        // Better off done at this level, since the rest of the related logic is here
        //_webSocket->setAdditonalQueryString(_connection->getAdditionalQueryString());
        _webSocket->setAddtionalHeaders(_connection->getAdditionalHttpHeaders());
#ifndef QT_NO_NETWORKPROXY
        _webSocket->setProxy(_connection->getProxySettings());
#endif
#ifndef QT_NO_SSL
        _webSocket->setSslConfiguration(_connection->getSslConfiguration());
#endif

        QString connectUrl(_connection->getWebSocketsUrl());

        if(_connection->useDefaultContextPaths())
            connectUrl += QString("/") + QString(_started ? "reconnect" : "connect");

        if(_connection->useDefaultQueryString())
            connectUrl += TransportHelper::getReceiveQueryString(_connection, getTransportType());

        if(!_connection->getAdditionalQueryString().isEmpty())
        {
            connectUrl += (_connection->useDefaultQueryString() ? "&" : "?");
            bool isFirst(true);
            foreach(auto pair, _connection->getAdditionalQueryString() )
            {
                if(isFirst) isFirst=false;
                else connectUrl += "&";
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 2)
                connectUrl += pair.first + "=" + pair.second.toHtmlEscaped();
#else
                connectUrl += pair.first + "=" + pair.second.toAscii();
#endif
            }
        }

        QUrl url(connectUrl);

        const QString scheme(url.scheme().toLower());
        url.setScheme( scheme == "wss" || scheme == "https" ? "wss" : "ws" );

        connect(_webSocket, SIGNAL(hostFound()), this, SLOT(onHostFound()));
        connect(_webSocket, SIGNAL(connected()), this, SLOT(onConnected()));
        connect(_webSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
        connect(_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

        connect(_webSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onTextMessageReceived(QString)));
        connect(_webSocket, SIGNAL(pong(quint64,QByteArray)), this, SLOT(onPong(quint64,QByteArray)));

        connect(_webSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onIgnoreSsl(QList<QSslError>)));

        connect(_webSocket, SIGNAL(debugMessageAvailable(QString)), this, SLOT(onDebugMessageAvailable(QString)));

        _connection->emitLogMessage("websocket open url: " + url.toDisplayString(), SignalR::Info);
        _webSocket->open(url);
    }
}

void WebSocketTransport::send(QString data)
{
    if(_webSocket)
    {
        qint64 bytesWritten = _webSocket->write(data);
        if(bytesWritten != data.size())
        {
            _connection->emitLogMessage("Written bytes does not equals given bytes", SignalR::Warning);
        }
    }
}

bool WebSocketTransport::abort(int timeoutMs)
{
    _handShakeCompleted = false;
    Q_UNUSED(timeoutMs);
    if(_webSocket)
    {
        _webSocket->close();
        _webSocket->deleteLater();
        _webSocket = 0;
    }
    return true;
}

void WebSocketTransport::retry()
{
    abort();
    start("");
}

const QString &WebSocketTransport::getTransportType()
{
    static QString type = "webSockets";
    return type;
}

bool WebSocketTransport::isHandShakeCompleted()
{ return _handShakeCompleted; }

void WebSocketTransport::lostConnection(ConnectionPrivate *con)
{
    HttpBasedTransport::lostConnection(con);
    retry();
}

void WebSocketTransport::onHostFound()
{
    _connection->emitLogMessage("WebSocket: Host found", SignalR::Info);
}

void WebSocketTransport::onConnected()
{
    QSharedPointer<SignalException> e;
    Q_EMIT transportStarted(e);
    _started = true;

    bool isVerToDblOk;
    const int protocolVersion((int)_connection->getProtocolVersion().toDouble(&isVerToDblOk));
    const QByteArray recordSeperator("\u001E");
    const QVariantMap handshakeRequestMap({
          {"protocol", "json"}
        , {"version", protocolVersion}
    });
    const QByteArray handshakeRequest(
        QJsonDocument::fromVariant(handshakeRequestMap).toJson(QJsonDocument::Compact)
        + recordSeperator );
    const int bytesWritten(_webSocket->write(handshakeRequest));
    const bool isFlushed(_webSocket->flush());
    if( bytesWritten == handshakeRequest.size() && isFlushed )
    {
        _shakingHands = true;
        _connection->emitLogMessage("WebSocketTransport handshake sent: "
            + QString(handshakeRequest), SignalR::Info);
    }
    else
    {
        _shakingHands = false;
        QSharedPointer<SignalException> error(
            WebSocketTransport::toSignalException(
                QAbstractSocket::SocketError::OperationError,
                "WebSocket write error" ) );
        _connection->onError(error);
    }
}

void WebSocketTransport::onDisconnected()
{
    if(!_webSocket)
    {
        return;
    }
    QSharedPointer<SignalException> error(
        WebSocketTransport::toSignalException(
            _webSocket->error(), _webSocket->errorString() ) );

    if(_webSocket->state() == QAbstractSocket::ConnectedState)
        _webSocket->close();

    _connection->onError(error);

    if(_connection->ensureReconnecting())
    {
        _connection->emitLogMessage("WS: lost connection, try to reconnect in " + QString::number(_connection->getReconnectWaitTime()) + "ms", SignalR::Debug);

        connect(&_retryTimerTimeout, SIGNAL(timeout()), this, SLOT(reconnectTimerTick()));
        _retryTimerTimeout.setInterval(_connection->getReconnectWaitTime());
        _retryTimerTimeout.start();

    }
    else if(_connection->getAutoReconnect())
    {
        _connection->emitLogMessage("WebSocket: lost connection, try to reconnect in " + QString::number(_connection->getReconnectWaitTime()) + "ms", SignalR::Debug);

        connect(&_retryTimerTimeout, SIGNAL(timeout()), this, SLOT(reconnectTimerTick()));
        _retryTimerTimeout.setInterval(_connection->getReconnectWaitTime());
        _retryTimerTimeout.start();

        return;
    }
}


void WebSocketTransport::reconnectTimerTick()
{
    _retryTimerTimeout.stop();
    disconnect(&_retryTimerTimeout, SIGNAL(timeout()), this, SLOT(reconnectTimerTick()));
    _connection->changeState(SignalR::Connected, SignalR::Reconnecting);


    start("");
}

#ifndef QT_NO_SSL
void WebSocketTransport::onIgnoreSsl(QList<QSslError> errors)
{
    if(!_connection->ignoreSslErrors())
        return;

    _webSocket->ignoreSslErrors(errors);

    foreach(QSslError er, errors)
    {
        _connection->emitLogMessage(er.errorString(), SignalR::Error);
    }
}
#endif

QSharedPointer<SignalException> WebSocketTransport::toSignalException(
    const QAbstractSocket::SocketError &er, const QString &msg )
{
    QSharedPointer<SignalException> error;
    switch(er)
    {
    case QAbstractSocket::RemoteHostClosedError:
        error = QSharedPointer<SignalException>(new SignalException(msg, SignalException::RemoteHostClosedConnection));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        error = QSharedPointer<SignalException>(new SignalException(msg, SignalException::ConnectionRefusedError));
        break;
    case QAbstractSocket::NetworkError:
        error = QSharedPointer<SignalException>(new SignalException(msg, SignalException::UnknownNetworkError));
        break;
    case QAbstractSocket::SocketAccessError:
    case QAbstractSocket::SocketResourceError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::DatagramTooLargeError:
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::SocketAddressNotAvailableError:
    case QAbstractSocket::UnsupportedSocketOperationError:
    case QAbstractSocket::UnfinishedSocketOperationError:
    case QAbstractSocket::ProxyAuthenticationRequiredError:
    case QAbstractSocket::SslHandshakeFailedError:
    case QAbstractSocket::ProxyConnectionRefusedError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:
    case QAbstractSocket::ProxyNotFoundError:
    case QAbstractSocket::ProxyProtocolError:
    case QAbstractSocket::UnknownSocketError:
    case QAbstractSocket::HostNotFoundError:
    #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 2)
    case QAbstractSocket::OperationError:
    case QAbstractSocket::SslInternalError:
    case QAbstractSocket::SslInvalidUserDataError:
    case QAbstractSocket::TemporaryError:
    #endif
        error = QSharedPointer<SignalException>(new SignalException(msg, SignalException::UnkownError));
        break;
    }
    return error;
}

void WebSocketTransport::onError(QAbstractSocket::SocketError)
{
    // Original
    //_connection->emitLogMessage(_webSocket->errorString(), SignalR::Warning);

    // Revised
    QSharedPointer<SignalException> error(
        WebSocketTransport::toSignalException(
            _webSocket->error(), _webSocket->errorString() ) );
    _connection->onError( error );
}

void WebSocketTransport::onTextMessageReceived(QString str)
{
    _connection->emitLogMessage("WebSocket: Message received: " + str, SignalR::Debug);

    bool timedOut = false, disconnected = false;
    quint64 messageId = 0;
    _connection->updateLastKeepAlive();

    if(_connection->getState() != SignalR::Connected)
        _connection->changeState(_connection->getState(), SignalR::Connected);

    QSharedPointer<SignalException> e = TransportHelper::processMessages(_connection, str, &timedOut, &disconnected, &messageId);

    if(e.isNull())
    {
        if( _shakingHands )
        {
            _handShakeCompleted = true;
            Q_EMIT handShakeCompleted();
        }

    }
    else _connection->onError(e);

    // TODO: emitting a "sent" signal on "receive"?!
    // This is a horribly misleading signal name!
    Q_EMIT onMessageSentCompleted(e, messageId);

    _shakingHands = false;
}

void WebSocketTransport::onPong(quint64, QByteArray)
{
    _connection->emitLogMessage("on pong", SignalR::Debug);
}

void WebSocketTransport::onDebugMessageAvailable(QString message)
{
    _connection->emitLogMessage(message, SignalR::Debug);
}

}}}
