#include "networking.h"

#include <ctime>

using namespace Telegram;

Networking::Networking(const QString &token, QObject *parent) :
    QObject(parent),
    m_nam(new QNetworkAccessManager(this)),
    m_token(token)
{
    connect(m_nam, SIGNAL(finished(QNetworkReply*)),
            this, SIGNAL(requestFinished(QNetworkReply*)));
}

Networking::~Networking()
{
    disconnect(m_nam, SIGNAL(finished(QNetworkReply*)),
               this, SIGNAL(requestFinished(QNetworkReply*)));
    delete m_nam;
}

QNetworkReply* Networking::asyncRequest(const QString &endpoint, const ParameterList &params, Networking::Method method)
{
    if (endpoint.isEmpty()) {
        qWarning("Cannot do request without endpoint");
        return 0;
    }
    if (m_token.isEmpty()) {
        qWarning("Cannot do request without a Telegram Bot Token");
        return 0;
    }

    QNetworkRequest req;
    QUrl url = buildUrl(endpoint);
    req.setUrl(url);

#ifdef DEBUG
    qDebug("HTTP request: %s", qUtf8Printable(req.url().toString()));
#endif

    QNetworkReply *reply = 0;

    if (method == GET) {
        url.setQuery(parameterListToString(params));
        req.setUrl(url);
        reply = m_nam->get(req);
    } else if (method == POST) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        reply = m_nam->post(req, parameterListToString(params));
    } else if (method == UPLOAD) {
        QByteArray boundary = generateMultipartBoundary(params);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + boundary);
        QByteArray requestData = generateMultipartFormData(params, boundary);
        req.setHeader(QNetworkRequest::ContentLengthHeader, requestData.length());
        reply = m_nam->post(req, requestData);
    } else {
        qCritical("No valid method!");        
    }

    if (reply == NULL) {
        qWarning("Reply is NULL");
    }

    return reply;
}

QUrl Networking::buildUrl(QString endpoint)
{
    QUrl url = QUrl();
    url.setScheme("https");
    url.setHost(API_HOST);
    url.setPath("/bot" + m_token + endpoint);

    return url;
}

QByteArray Networking::parameterListToString(const ParameterList &list)
{
    QByteArray ret;

    ParameterList::const_iterator i = list.begin();
    while (i != list.end()) {
        ret.append(i.key() + "=" + i.value().value + "&");
        ++i;
    }
    ret = ret.left(ret.length() - 1);

    return ret;
}

QByteArray Networking::generateMultipartBoundary(const ParameterList &list)
{
    // Generates a boundary that is not existent in the data
    QByteArray result;

    srand((unsigned int) time(NULL));
    ParameterList::const_iterator i = list.begin();
    while (i != list.end()) {
        if (i.value().isFile) {
            while (result.isEmpty() || i.value().value.contains(result)) {
                result.append(generateRandomString(4));
            }
        }
        ++i;
    }

    return result;
}

QByteArray Networking::generateMultipartFormData(const ParameterList &list, const QByteArray &boundary)
{
    QByteArray result;

    ParameterList::const_iterator i = list.begin();
    while (i != list.end()) {
        HttpParameter param = i.value();
        result.append("--" + boundary + "\r\n");
        result.append("Content-Disposition: form-data; name=\"" + i.key());
        if (param.isFile) {
            result.append("\"; filename=\"" + param.filename);
        }
        result.append("\"\r\n");
        if (param.isFile) {
            result.append("Content-Type: " + param.mimeType + "\r\n");
        }
        result.append("\r\n");
        result.append(param.value);
        result.append("\r\n");

        ++i;
    }
    result.append("--" + boundary + "--");

    return result;
}

QString Networking::generateRandomString(int length)
{
    static const std::string chars("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890-=[]\\;',./!@#$%^&*()_+{}|:\"<>?`~");
    static const size_t charsLen = chars.length();
    QString result;
    for (int i = 0; i < length; ++i) {
        result += QChar(chars[rand() % charsLen]);
    }
    return result;
}
