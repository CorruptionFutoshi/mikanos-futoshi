#include "keyboard.hpp"

#include <memory>
#include "usb/classdriver/keyboard.hpp"

namespace {
	const char keycode_map[256] = {
		0,	0,	0,	0,	'a',	'b',	'c',	'd',
		'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',
		'm',	'n',	'o',	'p',	'q',	'r',	's',	't',
		'u',	'v',	'w',	'x',	'y',	'z',	'1',	'2',
		'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
		'\n',	'\b',	0x08,	'\t',	' ',	'-',	'=',	'[',
		']',	'\\',	'#',	';',	'\'',	'`',	',',	'.',
		'/',	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	'/',	'*',	'-',	'+',
		'\n',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
		'8',	'9',	'0',	'.',	'\\',	0,	0,	'=',
	};


	const char keycode_map_shifted[256] = {
		0,	0,	0,	0,	'A',	'B',	'C',	'D',
		'E',	'F',	'G',	'H',	'I',	'J',	'K',	'L',
		'M',	'N',	'O',	'P',	'Q',	'R',	'S',	'T',
		'U',	'V',	'W',	'X',	'Y',	'Z',	'!',	'@',
		'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
		'\n',	'\b',	0x08,	'\t',	' ',	'_',	'+',	'{',
		'}',	'|',	'~',	':',	'"',	'~',	'<',	'>',
		'?',	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	'/',	'*',	'-',	'+',
		'\n',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
		'8',	'9',	'0',	'.',	'\\',	0,	0,	'=',
	};

	const int kLControlBitMask	= 0b00000001u;
	const int kLShiftBitMask	= 0b00000010u;
	const int kLAltBitMask		= 0b00000100u;
	const int kLGUIBitMask		= 0b00001000u;
	const int kRControlBitMask	= 0b00010000u;
	const int kRShiftBitMask	= 0b00100000u;
	const int kRAltBitMask		= 0b01000000u;
	const int kRGUIBitMask		= 0b10000000u;
}

void InitializeKeyboard(std::deque<Message>& msg_queue) {
	usb::HIDKeyboardDriver::default_observer = 
		[&msg_queue](uint8_t modifier, uint8_t keycode) {
			const bool shift = (modifier & (kLShiftBitMask | kRShiftBitMask)) != 0;
			char ascii = keycode_map[keycode];

			if (shift) {
				ascii = keycode_map_shifted[keycode];
			}

			Message msg{Message::kKeyPush};
			msg.arg.keyboard.modifier = modifier;
			msg.arg.keyboard.keycode = keycode;
			msg.arg.keyboard.ascii = ascii;
			msg_queue.push_back(msg);
		};
}
