/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <srs_librtmp.hpp>

#include <stdlib.h>

#include <string>
using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_protocol_rtmp.hpp>
#include <srs_lib_simple_socket.hpp>
#include <srs_kernel_log.hpp>
#include <srs_protocol_utility.hpp>

// if user want to define log, define the folowing macro.
#ifndef SRS_RTMP_USER_DEFINED_LOG
	// kernel module.
	ISrsLog* _srs_log = new ISrsLog();
	ISrsThreadContext* _srs_context = new ISrsThreadContext();
#endif

/**
* export runtime context.
*/
struct Context
{
	std::string url;
	std::string tcUrl;
	std::string host;
	std::string port;
	std::string vhost;
	std::string app;
	std::string stream;
	
    SrsRtmpClient* rtmp;
    SimpleSocketStream* skt;
    int stream_id;
    
    Context() {
        rtmp = NULL;
        skt = NULL;
        stream_id = 0;
    }
    virtual ~Context() {
        srs_freep(rtmp);
        srs_freep(skt);
    }
};

int srs_librtmp_context_connect(Context* context) 
{
	int ret = ERROR_SUCCESS;
    
    // parse uri
	size_t pos = string::npos;
	string uri = context->url;
	// tcUrl, stream
	if ((pos = uri.rfind("/")) != string::npos) {
		context->stream = uri.substr(pos + 1);
		context->tcUrl = uri = uri.substr(0, pos);
	}
	// schema
	if ((pos = uri.find("rtmp://")) != string::npos) {
		uri = uri.substr(pos + 7);
	}
	// host/vhost/port
	if ((pos = uri.find(":")) != string::npos) {
		context->vhost = context->host = uri.substr(0, pos);
		uri = uri.substr(pos + 1);
		
		if ((pos = uri.find("/")) != string::npos) {
			context->port = uri.substr(0, pos);
			uri = uri.substr(pos + 1);
		}
	} else {
		if ((pos = uri.find("/")) != string::npos) {
			context->vhost = context->host = uri.substr(0, pos);
			uri = uri.substr(pos + 1);
		}
		context->port = RTMP_DEFAULT_PORT;
	}
	// app
	context->app = uri;
	// query of app
	if ((pos = uri.find("?")) != string::npos) {
		context->app = uri.substr(0, pos);
		string query = uri.substr(pos + 1);
		if ((pos = query.find("vhost=")) != string::npos) {
			context->vhost = query.substr(pos + 6);
			if ((pos = context->vhost.find("&")) != string::npos) {
				context->vhost = context->vhost.substr(pos);
			}
		}
	}
    
    // create socket
	srs_freep(context->skt);
	context->skt = new SimpleSocketStream();
	
	if ((ret = context->skt->create_socket()) != ERROR_SUCCESS) {
		return ret;
	}
	
	// connect to server:port
	string server = srs_dns_resolve(context->host);
	if (server.empty()) {
		return -1;
	}
	if ((ret = context->skt->connect(server.c_str(), ::atoi(context->port.c_str()))) != ERROR_SUCCESS) {
		return ret;
	}
	
	return ret;
}

#ifdef __cplusplus
extern "C"{
#endif

srs_rtmp_t srs_rtmp_create(const char* url)
{
    Context* context = new Context();
	context->url = url;
    return context;
}

void srs_rtmp_destroy(srs_rtmp_t rtmp)
{
    srs_assert(rtmp != NULL);
    Context* context = (Context*)rtmp;
    
    srs_freep(context);
}

int srs_simple_handshake(srs_rtmp_t rtmp)
{
	int ret = ERROR_SUCCESS;
	
    srs_assert(rtmp != NULL);
    Context* context = (Context*)rtmp;
    
    // parse uri, resolve host, connect to server:port
    if ((ret = srs_librtmp_context_connect(context)) != ERROR_SUCCESS) {
        return ret;
    }
	
	// simple handshake
	srs_freep(context->rtmp);
	context->rtmp = new SrsRtmpClient(context->skt);
	
	if ((ret = context->rtmp->simple_handshake()) != ERROR_SUCCESS) {
		return ret;
	}
	
	return ret;
}

int srs_complex_handshake(srs_rtmp_t rtmp)
{
#ifndef SRS_SSL
	return ERROR_RTMP_HS_SSL_REQUIRE;
#endif

	return ERROR_SUCCESS;
}

int srs_connect_app(srs_rtmp_t rtmp)
{
	return ERROR_SUCCESS;
}

int srs_play_stream(srs_rtmp_t rtmp)
{
	return ERROR_SUCCESS;
}

int srs_publish_stream(srs_rtmp_t rtmp)
{
	return ERROR_SUCCESS;
}

int srs_ssl_enabled()
{
#ifndef SRS_SSL
	return false;
#endif
	return true;
}

int srs_version_major()
{
	return ::atoi(VERSION_MAJOR);
}

int srs_version_minor()
{
	return ::atoi(VERSION_MINOR);
}

int srs_version_revision()
{
	return ::atoi(VERSION_REVISION);
}

#ifdef __cplusplus
}
#endif