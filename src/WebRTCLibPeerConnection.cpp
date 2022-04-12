/*************************************************************************/
/*  WebRTCLibPeerConnection.cpp                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "WebRTCLibPeerConnection.hpp"
#include "WebRTCLibDataChannel.hpp"

using namespace godot;
using namespace godot_webrtc;

#ifdef GDNATIVE_WEBRTC
#define MK_ERROR(m_err)                 \
	struct Castable##m_err {            \
		operator int64_t() {            \
			return GODOT_##m_err;       \
		}                               \
		operator godot::Error() {       \
			return godot::Error::m_err; \
		}                               \
		Castable##m_err() {             \
		}                               \
	};

MK_ERROR(OK);
#define OK CastableOK()
MK_ERROR(FAILED);
#define FAILED CastableFAILED()
MK_ERROR(ERR_UNCONFIGURED);
#define ERR_UNCONFIGURED CastableERR_UNCONFIGURED()
MK_ERROR(ERR_UNAVAILABLE);
#define ERR_UNAVAILABLE CastableERR_UNAVAILABLE()
MK_ERROR(ERR_INVALID_PARAMETER);
#define ERR_INVALID_PARAMETER CastableERR_INVALID_PARAMETER()
MK_ERROR(ERR_BUG);
#define ERR_BUG CastableERR_BUG()

#define DICT_GET(p_dict, p_key) p_dict[p_key]

#else
#define DICT_GET(p_dict, p_key) p_dict.get(p_key, nil)
#endif

void WebRTCLibPeerConnection::initialize_signaling() {
	initKvsWebRtc();
}

void WebRTCLibPeerConnection::deinitialize_signaling() {
	deinitKvsWebRtc();
}

Error WebRTCLibPeerConnection::_parse_ice_server(RtcConfiguration &r_config, Dictionary p_server) {
	// TODO
#if 0
	Variant v;
	webrtc::PeerConnectionInterface::IceServer ice_server;
	String url;

	ERR_FAIL_COND_V(!p_server.has("urls"), ERR_INVALID_PARAMETER);

	// Parse mandatory URL
	Variant nil;
	v = DICT_GET(p_server, "urls");
	if (v.get_type() == Variant::STRING) {
		url = v;
		ice_server.urls.push_back(url.utf8().get_data());
	} else if (v.get_type() == Variant::ARRAY) {
		Array names = v;
		for (int j = 0; j < names.size(); j++) {
			v = names[j];
			ERR_FAIL_COND_V(v.get_type() != Variant::STRING, ERR_INVALID_PARAMETER);
			url = v;
			ice_server.urls.push_back(url.utf8().get_data());
		}
	} else {
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	}
	// Parse credentials (only meaningful for TURN, only support password)
	if (p_server.has("username") && (v = DICT_GET(p_server, "username")) && v.get_type() == Variant::STRING) {
		ice_server.username = (v.operator String()).utf8().get_data();
	}
	if (p_server.has("credential") && (v = DICT_GET(p_server, "credential")) && v.get_type() == Variant::STRING) {
		ice_server.password = (v.operator String()).utf8().get_data();
	}

	r_config.servers.push_back(ice_server);
#endif
	return OK;
}

Error WebRTCLibPeerConnection::_parse_channel_config(RtcDataChannelInit &r_config, const Dictionary &p_dict) {
	Variant nil;
	Variant v;
#define _SET_N(PROP, PNAME, TYPE)          \
	if (p_dict.has(#PROP)) {               \
		v = DICT_GET(p_dict, #PROP);       \
		if (v.get_type() == Variant::TYPE) \
			r_config.PNAME = v;            \
	}
#define _SET(PROP, TYPE) _SET_N(PROP, PROP, TYPE)
	// TODO FIXME check supported?!?
	_SET(negotiated, BOOL);
	//_SET(id, INT); // TODO Why missing?
	//_SET(maxPacketLifeTime, INT);
	//_SET(maxRetransmits, INT);
	_SET(ordered, BOOL);
#undef _SET
	if (p_dict.has("protocol") && (v = DICT_GET(p_dict, "protocol")) && v.get_type() == Variant::STRING) {
		// TODO
		//r_config.protocol = v.operator String().utf8().get_data();
	}

	// ID makes sense only when negotiated is true (and must be set in that case)
	// FIXME supported?
	//ERR_FAIL_COND_V(r_config.negotiated ? r_config.id == -1 : r_config>id != -1, ERR_INVALID_PARAMETER);
	// Only one of maxRetransmits and maxRetransmitTime can be set on a channel.
	//ERR_FAIL_COND_V(r_config.maxRetransmits && r_config.maxRetransmitTime, ERR_INVALID_PARAMETER);
	return OK;
}

void _on_ice_candidate(UINT64 p_user, PCHAR p_candidate) {
	if (!p_candidate) {
		return;
	}
	RtcIceCandidateInit session;
	memset(&session, 0, sizeof(session));
	WARN_PRINT(godot::String(p_candidate));
	deserializeRtcIceCandidateInit(p_candidate, strlen(p_candidate), &session);
	//godot::JSON *json = godot::JSON::get_singleton();
	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(p_candidate);
	ERR_FAIL_COND(err != OK);
	Variant result = json->get_data();
	ERR_FAIL_COND(result.get_type() != godot::Variant::DICTIONARY);
	Dictionary dict = result;
	ERR_FAIL_COND(!dict.has("candidate"));
	ERR_FAIL_COND(!dict.has("sdpMLineIndex"));
	ERR_FAIL_COND(!dict.has("sdpMid"));

	godot::String sdp_candidate = dict["candidate"];
	int sdp_mline = dict["sdpMLineIndex"];
	godot::String sdp_mid = dict["sdpMid"];
	((WebRTCLibPeerConnection *)p_user)->queue_signal("ice_candidate_created", 3, sdp_mid, sdp_mline, sdp_candidate);
}

void _on_data_channel(UINT64 p_user, RtcDataChannel *p_channel) {
	WARN_PRINT("data channel received");
	((WebRTCLibPeerConnection *)p_user)->queue_signal("data_channel_received", 1, WebRTCLibDataChannel::new_data_channel(p_channel));
}

void _on_connection_state_change(UINT64 p_user, RTC_PEER_CONNECTION_STATE p_state) {
	WARN_PRINT("State: " + godot::String::num(p_state));
}

void WebRTCLibPeerConnection::queue_candidate(godot::String p_mid_name, int p_mline, godot::String p_candidate) {
	godot::Array data;
	data.push_back(p_mid_name);
	data.push_back(p_mline);
	data.push_back(p_candidate);
	candidates.push_back(data);
}

void WebRTCLibPeerConnection::emit_candidates() {
	while (candidates.size()) {
		godot::Array sdp = candidates.pop_front();
		queue_signal("ice_candidate_created", 3, sdp[0], sdp[1], sdp[2]);
	}
}

int64_t WebRTCLibPeerConnection::_get_connection_state() const {
#if 0
	ERR_FAIL_COND_V(peer_connection.get() == nullptr, STATE_CLOSED);

	webrtc::PeerConnectionInterface::IceConnectionState state = peer_connection->ice_connection_state();
	switch (state) {
		case webrtc::PeerConnectionInterface::kIceConnectionNew:
			return STATE_NEW;
		case webrtc::PeerConnectionInterface::kIceConnectionChecking:
			return STATE_CONNECTING;
		case webrtc::PeerConnectionInterface::kIceConnectionConnected:
			return STATE_CONNECTED;
		case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
			return STATE_CONNECTED;
		case webrtc::PeerConnectionInterface::kIceConnectionFailed:
			return STATE_FAILED;
		case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
			return STATE_DISCONNECTED;
		case webrtc::PeerConnectionInterface::kIceConnectionClosed:
			return STATE_CLOSED;
		default:
			return STATE_CLOSED;
	}
#endif
	return STATE_CLOSED;
}

int64_t WebRTCLibPeerConnection::_initialize(const Dictionary &p_config) {
	RtcConfiguration config;
	memset(&config, 0, sizeof(config));

	Variant nil;
	Variant v;
#if 0
	if (p_config.has("iceServers") && (v = DICT_GET(p_config, "iceServers")) && v.get_type() == Variant::ARRAY) {
		Array servers = v;
		for (int i = 0; i < servers.size(); i++) {
			v = servers[i];
			ERR_FAIL_COND_V(v.get_type() != Variant::DICTIONARY, ERR_INVALID_PARAMETER);
			Dictionary server = v;
			Error err = _parse_ice_server(config, server);
			ERR_FAIL_COND_V(err != OK, FAILED);
		}
	}
#endif
	return (int64_t)_create_pc(config);
}

Object *WebRTCLibPeerConnection::_create_data_channel(const String &p_channel, const Dictionary &p_channel_config) {
	ERR_FAIL_COND_V(!peer_connection, nullptr);

	// Read config from dictionary
	RtcDataChannelInit config;
	memset(&config, 0, sizeof(config));

	Error err = _parse_channel_config(config, p_channel_config);
	ERR_FAIL_COND_V(err != OK, nullptr);

	RtcDataChannel *ch = nullptr;
	STATUS status = createDataChannel(peer_connection, (PCHAR)p_channel.utf8().get_data(), &config, &ch);
	ERR_FAIL_COND_V(status != STATUS_SUCCESS, nullptr);

	WebRTCLibDataChannel *wrapper = WebRTCLibDataChannel::new_data_channel(ch);
	ERR_FAIL_COND_V(wrapper == nullptr, nullptr);
	return wrapper;
}

int64_t WebRTCLibPeerConnection::_create_offer() {
	ERR_FAIL_COND_V(!peer_connection, ERR_UNCONFIGURED);
	RtcSessionDescriptionInit session;
	memset(&session, 0, sizeof(session));
	STATUS err = createOffer(peer_connection, &session);
	if (err != STATUS_SUCCESS) {
		ERR_PRINT("createOffer failed with error " + String::num(err));
		ERR_FAIL_V(FAILED);
	}
	queue_signal("session_description_created", 2, "offer", String(session.sdp));
	return OK;
}

//#define _MAKE_DESC(TYPE, SDP, RTCERR) webrtc::CreateSessionDescription((String(TYPE) == String("offer") ? webrtc::SdpType::kOffer : webrtc::SdpType::kAnswer), SDP.utf8().get_data(), RTCERR)
#define _MAKE_DESC(TYPE, SDP)                                                                    \
	ERR_FAIL_COND_V(SDP.length() > MAX_SESSION_DESCRIPTION_INIT_SDP_LEN, ERR_INVALID_PARAMETER); \
	RtcSessionDescriptionInit session;                                                           \
	memset(&session, 0, sizeof(session));                                                        \
	session.type = String(TYPE) == "offer" ? SDP_TYPE_OFFER : SDP_TYPE_ANSWER;                   \
	memcpy(session.sdp, SDP.get_data(), SDP.length());
int64_t WebRTCLibPeerConnection::_set_remote_description(const String &type, const String &sdp) {
	ERR_FAIL_COND_V(!peer_connection, ERR_UNCONFIGURED);
	_MAKE_DESC(type, sdp.utf8());
	STATUS err = setRemoteDescription(peer_connection, &session);
	if (err != STATUS_SUCCESS) {
		ERR_PRINT("setRemoteDescription failed with error " + String::num(err));
		ERR_FAIL_V(FAILED);
	}
	RtcSessionDescriptionInit answer;
	memset(&answer, 0, sizeof(answer));
	if (session.type != SDP_TYPE_OFFER) {
		return OK;
	}
	err = createAnswer(peer_connection, &answer);
	if (err != STATUS_SUCCESS) {
		ERR_PRINT("createAnser failed with error " + String::num(err));
		ERR_FAIL_V(FAILED);
	}
	queue_signal("session_description_created", 2, "answer", String(session.sdp));
	return OK;
}

int64_t WebRTCLibPeerConnection::_set_local_description(const String &type, const String &sdp) {
	ERR_FAIL_COND_V(!peer_connection, ERR_UNCONFIGURED);
	_MAKE_DESC(type, sdp.utf8());
	STATUS err = setLocalDescription(peer_connection, &session);
	if (err != STATUS_SUCCESS) {
		ERR_PRINT("setLocalDescription failed with error " + String::num(err));
		ERR_FAIL_V(FAILED);
	}
	return OK;
}
#undef _MAKE_DESC

int64_t WebRTCLibPeerConnection::_add_ice_candidate(const String &sdpMidName, int64_t sdpMlineIndexName, const String &sdpName) {
	ERR_FAIL_COND_V(!peer_connection, ERR_UNCONFIGURED);
	WARN_PRINT(godot::String::num((uint64_t)this));

	godot::Dictionary dict;
	dict["candidate"] = godot::String(sdpName);
	dict["sdpMid"] = godot::String(sdpMidName);
	dict["sdpMLineIndex"] = sdpMlineIndexName;
	godot::String config = "{\"candidate\":\"" + godot::String(sdpName) + "\",\"sdpMid\":\"" + godot::String::num(sdpMlineIndexName) + "\",\"sdpMLineIndex\":" + sdpMidName + "}";
	WARN_PRINT(config);
	RtcIceCandidateInit session;
	STATUS err = deserializeRtcIceCandidateInit((char *)config.utf8().get_data(), config.utf8().length(), &session);
	ERR_FAIL_COND_V(err != STATUS_SUCCESS, ERR_INVALID_PARAMETER);

	err = addIceCandidate(peer_connection, session.candidate);
	ERR_FAIL_COND_V(err != STATUS_SUCCESS, FAILED);
	return OK;
}

int64_t WebRTCLibPeerConnection::_poll() {
	ERR_FAIL_COND_V(!peer_connection, ERR_UNCONFIGURED);

	while (!signal_queue.empty()) {
		mutex_signal_queue->lock();
		Signal signal = signal_queue.front();
		signal_queue.pop();
		mutex_signal_queue->unlock();
		signal.emit(this);
	}
	return OK;
}

void WebRTCLibPeerConnection::_close() {
	if (peer_connection != nullptr) {
		closePeerConnection(peer_connection);
		freePeerConnection(&peer_connection);
		peer_connection = nullptr;
	}

	peer_connection = nullptr;
	while (!signal_queue.empty()) {
		signal_queue.pop();
	}
}

void WebRTCLibPeerConnection::_init() {
#ifdef GDNATIVE_WEBRTC
	register_interface(&interface);
#endif
	// initialize variables:
	mutex_signal_queue = new std::mutex;

	RtcConfiguration config;
	memset(&config, 0, sizeof(config));
	_create_pc(config);
}

Error WebRTCLibPeerConnection::_create_pc(RtcConfiguration &r_config) {
	// TODO free old peerconnection.
	r_config.iceTransportPolicy = ICE_TRANSPORT_POLICY_ALL;
	STATUS err = createPeerConnection(&r_config, &peer_connection);
	if (err != STATUS_SUCCESS) {
		WARN_PRINT("Error creating PeerConnection: " + godot::String::num(err));
		return FAILED;
	}
	err = peerConnectionOnIceCandidate(peer_connection, (UINT64)this, _on_ice_candidate);
	ERR_FAIL_COND_V(err, FAILED);
	err = peerConnectionOnDataChannel(peer_connection, (UINT64)this, _on_data_channel);
	ERR_FAIL_COND_V(err, FAILED);
	err = peerConnectionOnConnectionStateChange(peer_connection, (UINT64)this, _on_connection_state_change);
	ERR_FAIL_COND_V(err, FAILED);

	return OK;
}

WebRTCLibPeerConnection::WebRTCLibPeerConnection() {
#ifndef GDNATIVE_WEBRTC
	_init();
#endif
}

WebRTCLibPeerConnection::~WebRTCLibPeerConnection() {
#ifdef GDNATIVE_WEBRTC
	if (_owner) {
		register_interface(nullptr);
	}
#endif
	_close();
	delete mutex_signal_queue;
}

void WebRTCLibPeerConnection::queue_signal(String p_name, int p_argc, const Variant &p_arg1, const Variant &p_arg2, const Variant &p_arg3) {
	mutex_signal_queue->lock();
	const Variant argv[3] = { p_arg1, p_arg2, p_arg3 };
	signal_queue.push(Signal(p_name, p_argc, argv));
	mutex_signal_queue->unlock();
}
