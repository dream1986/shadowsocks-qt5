#include <QDir>
#include "profiles.h"

Profiles::Profiles(const QString &file)
{
    setJSONFile(file);
}

Profiles::~Profiles()
{}

void Profiles::setJSONFile(const QString &file)
{
    m_file = QDir::toNativeSeparators(file);
    QFile JSONFile(m_file);

    if (!JSONFile.exists()) {
        qWarning("Warning: gui-config.json does not exist!");
        backend.clear();
        backendType.clear();
        m_index = -1;
        debugLog = false;
        autoStart = false;
        autoHide = false;
        return;
    }

    JSONFile.open(QIODevice::ReadOnly | QIODevice::Text);

    if (!JSONFile.isOpen()) {
        qWarning("Critical Error: cannot open gui-config.json!");
    }

    if(!JSONFile.isReadable()) {
        qWarning("Critical Error: cannot read gui-config.json!");
    }

    QJsonParseError pe;
    JSONDoc = QJsonDocument::fromJson(JSONFile.readAll(), &pe);

    if (pe.error != QJsonParseError::NoError) {
        qWarning() << pe.errorString();
    }

    if (JSONDoc.isEmpty()) {
        qDebug() << m_file;
        qWarning("Warning: JSON Document is empty!");
    }

    JSONObj = JSONDoc.object();
    CONFArray = JSONObj["configs"].toArray();
    profileList.clear();//clear list before

    for (QJsonArray::iterator it = CONFArray.begin(); it != CONFArray.end(); ++it) {
        QJsonObject json = (*it).toObject();
        SSProfile p;
        p.profileName = json["profile"].toString();
        p.server = json["server"].toString();
        p.password = json["password"].toString();
        p.server_port = json["server_port"].toString();
        p.local_addr = json["local_address"].toString();
        p.local_port = json["local_port"].toString();
        p.method = json["method"].toString().toUpper();//using Upper-case in GUI
        p.timeout = json["timeout"].toString();
        profileList.append(p);
    }
    m_index = JSONObj["index"].toInt();
    backend = JSONObj["backend"].toString();
    backendType = JSONObj["type"].toString();
    debugLog = JSONObj["debug"].toBool();
    autoHide = JSONObj["autoHide"].toBool();
    autoStart = JSONObj["autoStart"].toBool();
    JSONFile.close();
}

int Profiles::count()
{
    return profileList.count();
}

QStringList Profiles::getProfileList()
{
    QStringList s;
    for (int i = 0; i < profileList.count(); i++) {
        s.append(profileList.at(i).profileName);
    }
    return s;
}

SSProfile Profiles::getProfile(int index)
{
    return profileList.at(index);
}

SSProfile Profiles::lastProfile()
{
    return profileList.last();
}

void Profiles::addProfile(const QString &p)
{
    SSProfile n;
    n.profileName = p;
    profileList.append(n);

    QJsonObject json;
    json["profile"] = QJsonValue(n.profileName);
    CONFArray.append(QJsonValue(json));
}

void Profiles::addProfileFromSSURI(const QString &name, QString uri)
{
    SSProfile p;
    p.profileName = name;

    if (uri.startsWith("ss://")) {
        uri.remove(0, 5);
    }
    QStringList resultList = QString(QByteArray::fromBase64(uri.toLatin1())).split(':');
    p.method = resultList.first();
    p.server_port = resultList.last();
    QStringList ser = resultList.at(1).split('@');
    if (ser.size() < 1) {
        qWarning() << "Invalid Base64 String.";
    }
    else {
        p.server = ser.last();
        ser.removeLast();
        p.password = ser.join('@');//incase there is a '@' in password
    }

    QJsonObject json;
    json["profile"] = QJsonValue(p.profileName);
    json["server"] = QJsonValue(p.server);
    json["password"] = QJsonValue(p.password);
    json["method"] = QJsonValue(p.method.toLower());
    json["server_port"] = QJsonValue(p.server_port);
    profileList.append(p);
    CONFArray.append(QJsonValue(json));
}

void Profiles::saveProfile(int index, SSProfile &p)
{
    profileList.replace(index, p);

    QJsonObject json;
    json["profile"] = QJsonValue(p.profileName);
    json["server"] = QJsonValue(p.server);
    json["server_port"] = QJsonValue(p.server_port);
    json["password"] = QJsonValue(p.password);
    json["local_address"] = QJsonValue(p.local_addr);
    json["local_port"] = QJsonValue(p.local_port);
    json["method"] = QJsonValue(p.method.isEmpty() ? QString("table") : p.method.toLower());//lower-case in config
    json["timeout"] = QJsonValue(p.timeout);
    CONFArray.replace(index, QJsonValue(json));
}

void Profiles::deleteProfile(int index)
{
    profileList.removeAt(index);
    CONFArray.removeAt(index);
}

void Profiles::saveProfileToJSON()
{
    JSONObj["index"] = QJsonValue(m_index);
    JSONObj["backend"] = QJsonValue(backend);
    JSONObj["type"] = QJsonValue(backendType);
    JSONObj["debug"] = QJsonValue(debugLog);
    JSONObj["autoHide"] = QJsonValue(autoHide);
    JSONObj["autoStart"] = QJsonValue(autoStart);
    JSONObj["configs"] = QJsonValue(CONFArray);

    JSONDoc.setObject(JSONObj);

    QFile JSONFile(m_file);
    JSONFile.open(QIODevice::WriteOnly | QIODevice::Text);
    if (JSONFile.isWritable()) {
        JSONFile.write(JSONDoc.toJson());
    }
    else {
        qWarning() << "Warning: file is not writable!";
    }
    JSONFile.close();
}

void Profiles::setBackend(const QString &a)
{
#ifdef _WIN32
    if (getBackendTypeID() == 3) {
        QDir python(a);
        python.cdUp();
        QString s(python.absolutePath() + QString("/Scripts/sslocal-script.py"));
        if (QFile::exists(s)) {
            backend = QDir::toNativeSeparators(s);
        }
        return;
    }
#endif
    backend = QDir::toNativeSeparators(a);
}

QString Profiles::getBackend()
{
    return backend;
}

void Profiles::setBackendType(const QString &t)
{
    backendType = t;
}

QString Profiles::getBackendType()
{
    return backendType;
}

int Profiles::getBackendTypeID()
{
    if (backendType == QString("Shadowsocks-Nodejs")) {
        return 1;
    }
    else if (backendType == QString("Shadowsocks-Go")) {
        return 2;
    }
    else if (backendType == QString("Shadowsocks-Python")) {
        return 3;
    }
    else {
        return 0;//Shadowsocks-libev
    }
}

void Profiles::setIndex(int index)
{
    m_index = index;
}

int Profiles::getIndex()
{
    return m_index;
}

bool Profiles::isDebug()
{
    return debugLog;
}

void Profiles::setDebug(bool d)
{
    debugLog = d;
}

bool Profiles::isAutoStart()
{
    return autoStart;
}

void Profiles::setAutoStart(bool s)
{
    autoStart = s;
}

void Profiles::setAutoHide(bool h)
{
    autoHide = h;
}

bool Profiles::isAutoHide()
{
    return autoHide;
}

void Profiles::revert()
{
    setJSONFile(m_file);
}

bool Profiles::isValidate(const SSProfile &sp)
{
    bool ok;
    int port = sp.server_port.toInt(&ok);
    if (!ok || port < 1 || port > 65535) {
        return false;
    }
    port = sp.local_port.toInt(&ok);
    if (!ok || port < 1 || port > 65535) {
        return false;
    }

    //TODO: more accurate
    if (sp.server.isEmpty() || sp.local_addr.isEmpty() || sp.method.isEmpty() || sp.timeout.toInt() < 1 || backend.isEmpty()) {
        return false;
    }
    else
        return true;
}

int Profiles::detectBackendTypeID(const QString &filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString ident(file.readLine());
    if (ident.contains("node")) {
        return 1;
    }
    else if (ident.contains("python")) {
        return 3;
    }
    else {
        if (filename.contains("ss-local")) {
            return 0;
        }
        else
            return 2;
    }
}