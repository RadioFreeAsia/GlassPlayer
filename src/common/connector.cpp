// connector.cpp
//
// Abstract base class for streaming server connections.
//
//   (C) Copyright 2014-2015 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <time.h>

#include <QDateTime>
#include <QStringList>

#include "connector.h"
#include "glasslimits.h"
#include "logging.h"

Connector::Connector(QObject *parent)
  : QObject(parent)
{
  conn_server_username="source";
  conn_server_password="";
  conn_audio_bitrates.push_back(0);
  conn_stream_name="no name";
  conn_stream_description="unknown";
  conn_stream_url="";
  conn_stream_irc="";
  conn_stream_icq="";
  conn_stream_aim="";
  conn_stream_genre="unknown";
  conn_stream_public=true;
  conn_stream_metadata_enabled=true;
  conn_stream_metadata="";
  conn_host_hostname="";
  conn_host_port=0;
  conn_connected=false;
  conn_dropouts=0;
}


Connector::~Connector()
{
}


QString Connector::serverUsername() const
{
  return conn_server_username;
}


void Connector::setServerUsername(const QString &str)
{
  conn_server_username=str;
}


QString Connector::serverPassword() const
{
  return conn_server_password;
}


void Connector::setServerPassword(const QString &str)
{
  conn_server_password=str;
}


QUrl Connector::serverUrl() const
{
  return conn_server_url;
}


void Connector::setServerUrl(const QUrl &url)
{
  conn_server_url=url;
  uint16_t port=conn_server_url.port();
  if(port==65535) {
    conn_server_url.setPort(DEFAULT_SERVER_PORT);
  }
  if(conn_server_url.path().isEmpty()) {
    conn_server_url.setPath("/");
  }
}


QString Connector::serverMountpoint() const
{
  return conn_server_url.path();
}


unsigned Connector::audioBitrate() const
{
  return conn_audio_bitrates[0];
}


void Connector::setAudioBitrate(unsigned rate)
{
  conn_audio_bitrates.clear();
  conn_audio_bitrates.push_back(rate);
}


std::vector<unsigned> *Connector::audioBitrates()
{
  return &conn_audio_bitrates;
}


void Connector::setAudioBitrates(std::vector<unsigned> *rates)
{
  conn_audio_bitrates.clear();
  for(unsigned i=0;i<rates->size();i++) {
    conn_audio_bitrates.push_back(rates->at(i));
  }
}


QString Connector::streamName() const
{
  return conn_stream_name;
}


void Connector::setStreamName(const QString &str)
{
  conn_stream_name=str;
}


QString Connector::streamDescription() const
{
  return conn_stream_description;
}


void Connector::setStreamDescription(const QString &str)
{
  conn_stream_description=str;
}


QString Connector::streamUrl() const
{
  return conn_stream_url;
}


void Connector::setStreamUrl(const QString &str)
{
  conn_stream_url=str;
}


QString Connector::streamIrc() const
{
  return conn_stream_irc;
}


void Connector::setStreamIrc(const QString &str)
{
  conn_stream_irc=str;
}


QString Connector::streamIcq() const
{
  return conn_stream_icq;
}


void Connector::setStreamIcq(const QString &str)
{
  conn_stream_icq=str;
}


QString Connector::streamAim() const
{
  return conn_stream_aim;
}


void Connector::setStreamAim(const QString &str)
{
  conn_stream_aim=str;
}


QString Connector::streamGenre() const
{
  return conn_stream_genre;
}


void Connector::setStreamGenre(const QString &str)
{
  conn_stream_genre=str;
}


bool Connector::streamMetadataEnabled() const
{
  return conn_stream_metadata_enabled;
}


void Connector::setStreamMetadataEnabled(bool state)
{
  conn_stream_metadata_enabled=state;
}


QString Connector::streamMetadata() const
{
  return conn_stream_metadata;
}


bool Connector::streamPublic() const
{
  return conn_stream_public;
}


void Connector::setStreamPublic(bool state)
{
  conn_stream_public=state;
}


Codec::Type Connector::codecType() const
{
  return conn_codec_type;
}


bool Connector::isConnected() const
{
  return conn_connected;
}


void Connector::connectToServer()
{
  connectToHostConnector();
}


void Connector::stop()
{
  setConnected(false);
}


QString Connector::scriptUp() const
{
  return conn_script_up;
}


void Connector::setScriptUp(const QString &cmd)
{
  conn_script_up=cmd;
}


QString Connector::scriptDown() const
{
  return conn_script_down;
}


void Connector::setScriptDown(const QString &cmd)
{
  conn_script_down=cmd;
}


void Connector::getStats(QStringList *hdrs,QStringList *values)
{
  hdrs->push_back("ConnectorConnected");
  if(conn_connected) {
    values->push_back("Yes");
  }
  else {
    values->push_back("No");
  }

  hdrs->push_back("ConnectorUrl");
  values->push_back(conn_server_url.toString());

  hdrs->push_back("ConnectorDropouts");
  values->push_back(QString().sprintf("%u",conn_dropouts));

  loadStats(hdrs,values);
}


QString Connector::serverTypeText(Connector::ServerType type)
{
  QString ret=tr("Unknown");

  switch(type) {
  case Connector::HlsServer:
    ret=tr("HTTP Live Stream (HLS)");
    break;

  case Connector::XCastServer:
    ret=tr("IceCast or Shoutcast");
    break;

  case Connector::FileServer:
    ret=tr("File");
    break;

  case Connector::LastServer:
    break;
  }

  return ret;
}


QString Connector::optionKeyword(Connector::ServerType type)
{
  QString ret;

  switch(type) {
  case Connector::HlsServer:
    ret="hls";
    break;

  case Connector::XCastServer:
    ret="xcast";
    break;

  case Connector::FileServer:
    ret="file";
    break;

  case Connector::LastServer:
    break;
  }

  return ret;
}


Connector::ServerType Connector::serverType(const QString &key)
{
  Connector::ServerType ret=Connector::LastServer;

  for(int i=0;i<Connector::LastServer;i++) {
    if(optionKeyword((Connector::ServerType)i)==key.toLower()) {
      ret=(Connector::ServerType)i;
    }
  }

  return ret;
}


bool Connector:: acceptsContentType(Connector::ServerType type,
				    const QString &mimetype)
{
  bool ret=false;

  switch(type) {
  case Connector::XCastServer:
    ret=(mimetype.toLower()=="audio/mpeg")||
      (mimetype.toLower()=="audio/aacp");
    break;

  case Connector::HlsServer:    // We don't list any mimetypes here because
  case Connector::FileServer:   // ServerId handles it.
    break;

  case Connector::LastServer:
    break;
  }

  return ret;
}


QString Connector::subMountpointName(const QString &mntpt,unsigned bitrate)
{
  QStringList f0=mntpt.split(".");
  int offset=0;

  if((f0[f0.size()-1]=="m3u")||(f0[f0.size()-1]=="m3u8")) {
    offset=1;
  }
  f0.insert(f0.begin()+f0.size()-offset,QString().sprintf("%u",bitrate));
  return f0.join(".");
}


QString Connector::pathPart(const QString &fullpath)
{
  QStringList f0=fullpath.split("/");
  f0.erase(f0.begin()+f0.size()-1);
  return f0.join("/");
}


QString Connector::basePart(const QString &fullpath)
{
  QStringList f0=fullpath.split("/");
  return f0[f0.size()-1];
}


QDateTime Connector::xmlTimestamp(const QString &str)
{
  QDate date;
  QTime time;
  QDateTime ret;
  QString time_str;
  QString tz_str;
  int tz_offset=0;

  QStringList f0=str.split("T");
  if(f0.size()==2) {
    //
    // Extract Date
    //
    QStringList f1=f0[0].split("-");
    if(f1.size()==3) {
      date=QDate(f1[0].toInt(),f1[1].toInt(),f1[2].toInt());
    }

    //
    // Extract Time
    //
    f1=f0[1].split("+");
    if(f1.size()==2)  {
      time_str=f1[0];
      tz_str="+"+f1[1];
    }
    else {
      f1=f0[1].split("-");
      if(f1.size()==2)  {
	time_str=f1[0];
	tz_str="-"+f1[1];
      }
      else {
	if(f0[1].right(1).toLower()=="z") {  // UTC
	  time_str=f0[1].left(f0[1].length()-1);
	  tz_str="+00:00";
	}
	else {   // No TZ info, assume local time
	  time_str=f0[1];
	  tz_str="+00:00";
	}
      }
    }
    QStringList f2=time_str.split(":");
    if(f2.size()==3) {
      QStringList f3=f2[2].split(".");
      if(f3.size()==2) {
	time=QTime(f2[0].toInt(),f2[1].toInt(),f3[0].toInt(),f3[1].toInt());
      }
      else {
	time=QTime(f2[0].toInt(),f2[1].toInt(),f2[2].toInt());
      }
      f2=tz_str.right(tz_str.length()-1).split(":");
      if(f2.size()==2) {
	tz_offset=60*f2[0].toInt()+f2[1].toInt();
	if(tz_str.left(1)=="-") {
	  tz_offset=-tz_offset;
	}
      }
    }
  }
  if(date.isValid()&&time.isValid()) {
    ret=QDateTime(date,time);
    ret.addSecs(Connector::timezoneOffset()-tz_offset);
  }

  return ret;
}


QString Connector::timezoneOffsetString()
{
  QString ret="Z";
  time_t t=time(NULL);
  time_t gmt;
  time_t lt;
  
  gmt=mktime(gmtime(&t));
  lt=mktime(localtime(&t));

  if(gmt<lt) {
    ret=QString().sprintf("-%02ld:%02ld",(lt-gmt)/3600,((lt-gmt)%3600)/60);
  }
  if(gmt>lt) {
    ret=QString().sprintf("+%02ld:%02ld",(gmt-lt)/3600,((gmt-lt)%3600)/60);
  }

  return ret;
}


int Connector::timezoneOffset()
{
  time_t gmt;
  time_t lt;
  time_t t=time(NULL);
  
  gmt=mktime(gmtime(&t));
  lt=mktime(localtime(&t));

  return (gmt-lt)/60;
}


void Connector::setCodecType(Codec::Type type)
{
  conn_codec_type=type;
}


void Connector::setConnected(bool state)
{
  if(state!=conn_connected) {
    if(!state) {
      conn_dropouts++;
    }
    conn_connected=state;
    emit connected(conn_connected);
  }
}


QString Connector::urlEncode(const QString &str)
{
  QString ret;

  for(int i=0;i<str.length();i++) {
    if(str.at(i).isLetterOrNumber()) {
      ret+=str.mid(i,1);
    }
    else {
      ret+=QString().sprintf("%%%02X",str.at(i).toLatin1());
    }
  }

  return ret;
}


QString Connector::urlDecode(const QString &str)
{
  int istate=0;
  unsigned n;
  QString code;
  QString ret;
  bool ok=false;

  for(int i=0;i<str.length();i++) {
    switch(istate) {
    case 0:
      if(str.at(i)==QChar('+')) {
	ret+=" ";
      }
      else {
	if(str.at(i)==QChar('%')) {
	  istate=1;
	}
	else {
	  ret+=str.at(i);
	}
      }
      break;

    case 1:
      n=str.mid(i,1).toUInt(&ok);
      if((!ok)||(n>9)) {
	istate=0;
      }
      code=str.mid(i,1);
      istate=2;
      break;

    case 2:
      n=str.mid(i,1).toUInt(&ok);
      if((!ok)||(n>9)) {
	istate=0;
      }
      code+=str.mid(i,1);
      ret+=QChar(code.toInt(&ok,16));
      istate=0;
      break;
    }
  }

  return ret;
}


char base64_dict[64]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
		      'O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b',
		      'c','d','e','f','g','h','i','j','k','l','m','n','o','p',
		      'q','r','s','t','u','v','w','x','y','z','0','1','2','3',
		      '4','5','6','7','8','9','+','/'};


QString Connector::base64Encode(const QString &str)
{
  QString ret;
  uint32_t buf;

  //
  // Build Groups
  //
  for(int i=0;i<str.length();i+=3) {
    buf=(0xff&(str.at(i).toLatin1()))<<16;
    if((i+1)<str.length()) {
      buf|=(0xff&(str.at(i+1).toLatin1()))<<8;
      if((i+2)<str.length()) {
	buf|=0xff&(str.at(i+2).toLatin1());
      }
    }

    //
    // Dictionary Lookup
    //
    for(int i=3;i>=0;i--) {
      ret+=base64_dict[0x3f&(buf>>(6*i))];
    }
  }

  //
  // Apply Padding
  //
  switch(str.length()%3) {
    case 1:
      ret=ret.left(ret.length()-2)+"==";
      break;
      
    case 2:
      ret=ret.left(ret.length()-1)+"=";
      break;
  }


  return ret;
}


QString Connector::base64Decode(const QString &str,bool *ok)
{
  QString ret;
  uint32_t buf=0;
  char c;
  int pad=0;
  bool found=false;

  //
  // Set Status
  //
  if(ok!=NULL) {
    *ok=true;
  }

  //
  // Dictionary Lookup
  //
  for(int i=0;i<str.length();i+=4) {
    buf=0;
    for(int j=0;j<4;j++) {
      if(((i+j)<str.length())&&(c=str.at(i+j).toLatin1())!='=') {
	found=false;
	for(unsigned k=0;k<64;k++) {
	  if(base64_dict[k]==c) {
	    buf|=(k<<(6*(3-j)));
	    found=true;
	  }
	}
	if(!found) {  // Illegal character!
	  if(ok!=NULL) {
	    *ok=false;
	  }
	  return QString();
	}
      }
      else {
	if(str.at(i+j)=='=') {
	  pad++;
	}
      }
    }

    //
    // Extract Groups
    //
    for(int i=2;i>=0;i--) {
      ret+=(0xff&(buf>>(8*i)));
    }
  }

  //
  // Apply Padding
  //
  if(pad>2) {
    if(ok!=NULL) {
      *ok=false;
    }
    return QString();
  }
  ret=ret.left(ret.length()-pad);

  return ret;
}


QString Connector::curlStrError(int exit_code)
{
  //
  // From the curl(1) man page (v7.29.0)
  //
  QString ret=tr("Unknown CURL error");

  switch(exit_code) {
  case 1:
    ret=tr("Unsupported protocol");
    break;

  case 2:
    ret=tr("Failed to initialize");
    break;

  case 3:
    ret=tr("Malformed URL");
    break;

  case 4:
    ret=tr("Feature not supported");
    break;

  case 5:
    ret=tr("Cannot resolve proxy");
    break;

  case 6:
    ret=tr("Cannot resolve host");
    break;

  case 7:
    ret=tr("Connection to host failed");
    break;

  case 8:
    ret=tr("Unrecognized FTP server response");
    break;

  case 9:
    ret=tr("FTP access denied");
    break;

  case 11:
    ret=tr("Unrecognized FTP PASS reply");
    break;

  case 13:
    ret=tr("Unrecognized FTP PASV reply");
    break;

  case 14:
    ret=tr("Unrecognized FTP 227 format");
    break;

  case 15:
    ret=tr("FTP can't get host");
    break;

  case 17:
    ret=tr("FTP couldn't set binary mode");
    break;

  case 18:
    ret=tr("Partial file transfer");
    break;

  case 19:
    ret=tr("FTP download failed");
    break;

  case 21:
    ret=tr("FTP quote error");
    break;

  case 22:
    ret=tr("HTTP page not retrieved");
    break;

  case 23:
    ret=tr("Write error");
    break;

  case 25:
    ret=tr("FTP STOR error");
    break;

  case 26:
    ret=tr("Read error");
    break;

  case 27:
    ret=tr("Out of memory");
    break;

  case 28:
    ret=tr("Write error");
    break;

  case 30:
    ret=tr("FTP PORT failed");
    break;

  case 31:
    ret=tr("FTP REST failed");
    break;

  case 33:
    ret=tr("HTTP range error");
    break;

  case 34:
    ret=tr("HTTP POST error");
    break;

  case 35:
    ret=tr("SSL connect error");
    break;

  case 36:
    ret=tr("FTP bad download resume");
    break;

  case 37:
    ret=tr("FILE read failure");
    break;

  case 38:
    ret=tr("LDAP bind error");
    break;

  case 39:
    ret=tr("LDAP search failed");
    break;

  case 41:
    ret=tr("LDAP function not found");
    break;

  case 42:
    ret=tr("Aborted");
    break;

  case 43:
    ret=tr("Internal error");
    break;

  case 45:
    ret=tr("Interface error");
    break;

  case 47:
    ret=tr("Too many redirects");
    break;

  case 48:
    ret=tr("Unknown option");
    break;

  case 49:
    ret=tr("Malformed telnet option");
    break;

  case 51:
    ret=tr("Bad certificate");
    break;

  case 52:
    ret=tr("No data returned");
    break;

  case 53:
    ret=tr("No SSL crypto engine");
    break;

  case 54:
    ret=tr("Cannot set SSL crypto engine as default");
    break;

  case 55:
    ret=tr("Send failure");
    break;

  case 56:
    ret=tr("Receive failure");
    break;

  case 58:
    ret=tr("Local certificate problem");
    break;

  case 59:
    ret=tr("Cannot use requested SSL cipher");
    break;

  case 60:
    ret=tr("Peer certificate cannot be authenticated");
    break;

  case 61:
    ret=tr("Unrecognized transfer encoding");
    break;

  case 62:
    ret=tr("Invalid LDAP URL");
    break;

  case 63:
    ret=tr("Maximum file size exceeded");
    break;

  case 64:
    ret=tr("Requested FTP SSL level failed");
    break;

  case 65:
    ret=tr("Rewind failed");
    break;

  case 66:
    ret=tr("SSL engine initialization failed");
    break;

  case 67:
    ret=tr("Authentication failure");
    break;

  case 68:
    ret=tr("TFTP file not found");
    break;

  case 69:
    ret=tr("TFTP permmission problem");
    break;

  case 70:
    ret=tr("TFTP out of disc space");
    break;

  case 71:
    ret=tr("TFTP illegal operation");
    break;

  case 72:
    ret=tr("TFTP unkown transfer ID");
    break;

  case 73:
    ret=tr("TFTP file already exists");
    break;

  case 74:
    ret=tr("TFTP no such user");
    break;

  case 75:
    ret=tr("Character conversion failed");
    break;

  case 76:
    ret=tr("Character conversion functions required");
    break;

  case 77:
    ret=tr("SSL CA cert read problems");
    break;

  case 78:
    ret=tr("Reference resources does not exist");
    break;

  case 79:
    ret=tr("SSH unspecified error");
    break;

  case 80:
    ret=tr("SSL failed to shut down connection");
    break;

  case 82:
    ret=tr("Cannot load CRL file");
    break;

  case 83:
    ret=tr("Issuer check failed");
    break;

  case 84:
    ret=tr("FTP PRET command failed");
    break;

  case 85:
    ret=tr("RTSP CSeq mismatch");
    break;

  case 86:
    ret=tr("RTSP SSID mismatch");
    break;

  case 87:
    ret=tr("FTP unable to parse file list");
    break;

  case 88:
    ret=tr("FTP chunk callback error");
    break;

  default:
    ret=tr("Unknown CURL error")+QString().sprintf(" [%d]",exit_code);
  }

  return ret;
}


QString Connector::httpStrError(int status_code)
{
  //
  // From RFC-2616 Section 6.1.1
  //
  QString ret=tr("Unknown status code");

  switch(status_code) {
  case 100:
    ret=tr("Continue");
    break;
    
  case 101:
    ret=tr("Switching Protocols");
    break;
    
  case 200:
    ret=tr("OK");
    break;
    
  case 201:
    ret=tr("Created");
    break;
    
  case 202:
    ret=tr("Accepted");
    break;
    
  case 203:
    ret=tr("Non-Authoritative Information");
    break;
    
  case 204:
    ret=tr("No Content");
    break;
    
  case 205:
    ret=tr("Reset Content");
    break;
    
  case 206:
    ret=tr("Partial Content");
    break;
    
  case 300:
    ret=tr("Multiple Choices");
    break;
    
  case 301:
    ret=tr("Moved Permanently");
    break;
    
  case 302:
    ret=tr("Found");
    break;
    
  case 303:
    ret=tr("See Other");
    break;
    
  case 304:
    ret=tr("Not Modified");
    break;
    
  case 305:
    ret=tr("Use Proxy");
    break;
    
  case 307:
    ret=tr("Temporary Redirect");
    break;
    
  case 400:
    ret=tr("Bad Request");
    break;
    
  case 401:
    ret=tr("Unauthorized");
    break;
    
  case 402:
    ret=tr("Payment Required");
    break;
    
  case 403:
    ret=tr("Forbidden");
    break;
    
  case 404:
    ret=tr("Not Found");
    break;
    
  case 405:
    ret=tr("Method Not Allowed");
    break;
    
  case 406:
    ret=tr("Not Acceptable");
    break;
    
  case 407:
    ret=tr("Proxy Authentication Required");
    break;
    
  case 408:
    ret=tr("Request Time-out");
    break;
    
  case 409:
    ret=tr("Conflict");
    break;
    
  case 410:
    ret=tr("Gone");
    break;
    
  case 411:
    ret=tr("Length Required");
    break;
    
  case 412:
    ret=tr("Precondition Failed");
    break;
    
  case 413:
    ret=tr("Request Entity Too Large");
    break;
    
  case 414:
    ret=tr("Request-URI Too Large");
    break;
    
  case 415:
    ret=tr("Unsupported Media Type");
    break;
    
  case 416:
    ret=tr("Requested range not satisfiable");
    break;
    
  case 417:
    ret=tr("Expectation Failed");
    break;
    
  case 500:
    ret=tr("Internal Server Error");
    break;
    
  case 501:
    ret=tr("Not Implemented");
    break;
    
  case 502:
    ret=tr("Bad Gateway");
    break;
    
  case 503:
    ret=tr("Service Unavailable");
    break;
    
  case 504:
    ret=tr("Gateway Time-out");
    break;
    
  case 505:
    ret=tr("HTTP Version not supported");
    break;
  }

  return QString().sprintf("%d - ",status_code)+ret;
}


QString Connector::socketErrorText(QAbstractSocket::SocketError err)
{
  QString ret=tr("Unknown socket error")+QString().sprintf(" [%u]",err);

  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
    ret=tr("connection refused");
    break;

  case QAbstractSocket::RemoteHostClosedError:
    ret=tr("remote host closed connection");
    break;

  case QAbstractSocket::HostNotFoundError:
    ret=tr("host not found");
    break;

  case QAbstractSocket::SocketAccessError:
    ret=tr("socket access error");
    break;

  case QAbstractSocket::SocketTimeoutError:
    ret=tr("operation timed out");
    break;

  case QAbstractSocket::DatagramTooLargeError:
    ret=tr("datagram too large");
    break;

  case QAbstractSocket::NetworkError:
    ret=tr("network error");
    break;

  case QAbstractSocket::AddressInUseError:
    ret=tr("address in use");
    break;

  case QAbstractSocket::SocketAddressNotAvailableError:
    ret=tr("address not available");
    break;

  case QAbstractSocket::UnsupportedSocketOperationError:
    ret=tr("unsupported socket operation");
    break;

  default:
    break;
  }

  return ret;
}


QString Connector::processErrorText(QProcess::ProcessError err)
{
  QString ret=tr("Unknown process error")+QString().sprintf("[%u]",err);

  switch(err) {
  case QProcess::FailedToStart:
    ret=tr("failed to start");
    break;

  case QProcess::Crashed:
    ret=tr("crashed");
    break;

  case QProcess::Timedout:
    ret=tr("timed out");
    break;

  case QProcess::WriteError:
    ret=tr("write error");
    break;

  case QProcess::ReadError:
    ret=tr("read error");
    break;

  case QProcess::UnknownError:
    ret=tr("unknown error");
    break;
  }

  return ret;
}
