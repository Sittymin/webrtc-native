/*************************************************************************/
/*  WebRTCLibDataChannel.cpp                                             */
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

#include "WebRTCLibDataChannel.hpp"

#ifdef GDNATIVE_WEBRTC
#include "GDNativeLibrary.hpp"
#include "NativeScript.hpp"
#define ERR_UNAVAILABLE GODOT_ERR_UNAVAILABLE
#define FAILED GODOT_FAILED
#define OK GODOT_OK
#endif

#include <stdio.h>
#include <string.h>

using namespace godot;
using namespace godot_webrtc;

// DataChannel
WebRTCLibDataChannel *WebRTCLibDataChannel::new_data_channel(std::shared_ptr<rtc::DataChannel> p_channel) {
	// Invalid channel result in NULL return
	ERR_FAIL_COND_V(!p_channel, nullptr);

#ifdef GDNATIVE_WEBRTC
	// Instance a WebRTCDataChannelGDNative object
	WebRTCDataChannelGDNative *native = WebRTCDataChannelGDNative::_new();
	// Set our implementation as it's script
	NativeScript *script = NativeScript::_new();
	script->set_library(detail::get_wrapper<GDNativeLibrary>((godot_object *)gdnlib));
	script->set_class_name("WebRTCLibDataChannel");
	native->set_script(script);
	WebRTCLibDataChannel *out = native->cast_to<WebRTCLibDataChannel>(native);
#else
	WebRTCLibDataChannel *out = memnew(WebRTCLibDataChannel);
#endif
	// Bind the library data channel to our object.
	out->bind_channel(p_channel);
	return out;
}

void WebRTCLibDataChannel::bind_channel(std::shared_ptr<rtc::DataChannel> p_channel) {
	ERR_FAIL_COND(!p_channel);

	channel = p_channel;

	// TODO "this" is not correct. "this" make memory go boom!
	p_channel->onMessage([this](auto message) {
		if (std::holds_alternative<rtc::string>(message)) {
			rtc::string str = std::get<rtc::string>(message);
			queue_packet(reinterpret_cast<const uint8_t *>(str.c_str()), str.size());
		} else if (std::holds_alternative<rtc::binary>(message)) {
			rtc::binary bin = std::get<rtc::binary>(message);
			queue_packet(reinterpret_cast<const uint8_t *>(&bin[0]), bin.size());
		} else {
			ERR_PRINT("Message parsing bug. Unknown message type.");
		}
	});
	/*
	p_channel->onOpen([]() {
		std::cout << "Open" << std::endl;
	});
	p_channel->onClosed([]() {
		std::cout << "Closed" << std::endl;
	});
	*/
	p_channel->onError([](auto error) {
		ERR_PRINT("Channel Error: " + String(std::string(error).c_str()));
	});
}

void WebRTCLibDataChannel::queue_packet(const uint8_t *data, uint32_t size) {
	mutex->lock();

	std::vector<uint8_t> packet;
	packet.resize(size);
	memcpy(&packet[0], data, size);
	packet_queue.push(packet);

	mutex->unlock();
}

void WebRTCLibDataChannel::_set_write_mode(int64_t mode) {
	// TODO
}

int64_t WebRTCLibDataChannel::_get_write_mode() const {
	return 0; // TODO
}

bool WebRTCLibDataChannel::_was_string_packet() const {
	return false; // TODO
}

int64_t WebRTCLibDataChannel::_get_ready_state() const {
	ERR_FAIL_COND_V(!channel, STATE_CLOSED);
	// TODO opening/closing.
	return channel->isOpen() ? STATE_OPEN : STATE_CLOSED;
}

String WebRTCLibDataChannel::_get_label() const {
	ERR_FAIL_COND_V(!channel, "");
	return channel->label().c_str();
}

bool WebRTCLibDataChannel::_is_ordered() const {
	ERR_FAIL_COND_V(!channel, false);
	return channel->reliability().unordered == false;
}

int64_t WebRTCLibDataChannel::_get_id() const {
	ERR_FAIL_COND_V(!channel, -1);
	return channel->id();
}

int64_t WebRTCLibDataChannel::_get_max_packet_life_time() const {
	ERR_FAIL_COND_V(!channel, 0);
	return channel->reliability().type == rtc::Reliability::Type::Timed ? std::get<std::chrono::milliseconds>(channel->reliability().rexmit).count() : -1;
}

int64_t WebRTCLibDataChannel::_get_max_retransmits() const {
	ERR_FAIL_COND_V(!channel, 0);
	return channel->reliability().type == rtc::Reliability::Type::Rexmit ? std::get<int>(channel->reliability().rexmit) : -1;
}

String WebRTCLibDataChannel::_get_protocol() const {
	ERR_FAIL_COND_V(!channel, "");
	return channel->protocol().c_str();
}

bool WebRTCLibDataChannel::_is_negotiated() const {
	ERR_FAIL_COND_V(!channel, false);
	return false; // TODO
}

int64_t WebRTCLibDataChannel::_get_buffered_amount() const {
	ERR_FAIL_COND_V(!channel, 0);
	return channel->bufferedAmount();
}

int64_t WebRTCLibDataChannel::_poll() {
	return OK;
}

void WebRTCLibDataChannel::_close() try {
	if (channel) {
		channel->close();
	}
} catch (...) { /* */ }

int64_t WebRTCLibDataChannel::_get_packet(const uint8_t **r_buffer, int32_t *r_len) {
	ERR_FAIL_COND_V(packet_queue.empty(), ERR_UNAVAILABLE);

	mutex->lock();

	// Update current packet and pop queue
	current_packet = packet_queue.front();
	packet_queue.pop();
	// Set out buffer and size (buffer will be gone at next get_packet or close)
	*r_buffer = &current_packet[0];
	*r_len = current_packet.size();

	mutex->unlock();

	return 0;
}

int64_t WebRTCLibDataChannel::_put_packet(const uint8_t *p_buffer, int64_t p_len) try {
	ERR_FAIL_COND_V(!channel, FAILED);
	ERR_FAIL_COND_V(channel->isClosed(), FAILED);
	channel->send(reinterpret_cast<const std::byte *>(p_buffer), p_len);
	return OK;
} catch (const std::exception &e) {
	ERR_PRINT(e.what());
	ERR_FAIL_V(FAILED);
}


int64_t WebRTCLibDataChannel::_get_available_packet_count() const {
	return packet_queue.size();
}

int64_t WebRTCLibDataChannel::_get_max_packet_size() const {
	return 1200; // TODO
}

WebRTCLibDataChannel::WebRTCLibDataChannel() {
	mutex = new std::mutex;
}

WebRTCLibDataChannel::~WebRTCLibDataChannel() {
	_close();
	channel = nullptr;
	delete mutex;
}
