/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/formats/xmlparser.h"
#include "common/archive.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/system.h"

namespace Common {

XMLParser::~XMLParser() {
	while (!_activeKey.empty())
		freeNode(_activeKey.pop());

	delete _XMLkeys;
	delete _stream;

	for (auto *layout : _layoutList)
		delete layout;

	_layoutList.clear();
}

bool XMLParser::loadFile(const Path &filename) {
	_stream = SearchMan.createReadStreamForMember(filename);
	if (!_stream)
		return false;

	_fileName = filename;
	return true;
}

bool XMLParser::loadFile(const FSNode &node) {
	_stream = node.createReadStream();
	if (!_stream)
		return false;

	_fileName = node.getName();
	return true;
}

bool XMLParser::loadBuffer(const byte *buffer, uint32 size, DisposeAfterUse::Flag disposable) {
	_stream = new MemoryReadStream(buffer, size, disposable);
	_fileName = "Memory Stream";
	return true;
}

bool XMLParser::loadStream(SeekableReadStream *stream, const String &name) {
	_stream = stream;
	_fileName = name;
	return _stream != nullptr;
}

void XMLParser::close() {
	delete _stream;
	_stream = nullptr;
}

bool XMLParser::parserError(const String &errStr) {
	_state = kParserError;

	const int startPosition = _stream->pos();
	int currentPosition = startPosition;
	int lineCount = 1;
	char c = 0;

	_stream->seek(0, SEEK_SET);

	while (currentPosition--) {
		c = _stream->readByte();

		if (c == '\n' || c == '\r')
			lineCount++;
	}

	assert(_stream->pos() == startPosition);
	currentPosition = startPosition;

	Common::String errorMessage = Common::String::format("\n  File <%s>, line %d:\n", _fileName.toString().c_str(), lineCount);

	if (startPosition > 1) {
		int keyOpening = 0;
		int keyClosing = 0;

		while (currentPosition-- && keyOpening == 0) {
			_stream->seek(-2, SEEK_CUR);
			c = _stream->readByte();

			if (c == '<')
				keyOpening = currentPosition - 1;
			else if (c == '>')
				keyClosing = currentPosition;
		}

		_stream->seek(startPosition, SEEK_SET);
		currentPosition = startPosition;
		while (keyClosing == 0 && c && currentPosition++) {
			c = _stream->readByte();

			if (c == '>')
				keyClosing = currentPosition;
		}

		currentPosition = (keyClosing - keyOpening);
		_stream->seek(keyOpening, SEEK_SET);

		while (currentPosition--)
			errorMessage += (char)_stream->readByte();
	}

	errorMessage += "\n\nParser error: ";
	errorMessage += errStr;
	errorMessage += "\n\n";

	g_system->logMessage(LogMessageType::kError, errorMessage.c_str());

	return false;
}

bool XMLParser::parseXMLHeader(ParserNode *node) {
	assert(node->header);

	if (_activeKey.size() != 1)
		return parserError("XML Header is expected in the global scope.");

	if (!node->values.contains("version"))
		return parserError("Missing XML version in XML header.");

	if (node->values["version"] != "1.0")
		return parserError("Unsupported XML version.");

	return true;
}

bool XMLParser::parseActiveKey(bool closed) {
	bool ignore = false;
	assert(_activeKey.empty() == false);

	ParserNode *key = _activeKey.top();

	if (key->name == "xml" && key->header == true) {
		assert(closed);
		return parseXMLHeader(key) && closeKey();
	}

	XMLKeyLayout *layout = (_activeKey.size() == 1) ? _XMLkeys : getParentNode(key)->layout;

	if (layout->children.contains(key->name)) {
		key->layout = layout->children[key->name];

		StringMap localMap = key->values;
		int keyCount = localMap.size();

		for (const auto &prop : key->layout->properties) {
			if (prop.required && !localMap.contains(prop.name))
				return parserError("Missing required property '" + prop.name + "' inside key '" + key->name + "'");
			else if (localMap.contains(prop.name)) {
				keyCount--;
				localMap.erase(prop.name);
			}
		}

		if (keyCount > 0) {
			Common::String missingKeys;

			for (auto i = localMap.begin(); i != localMap.end(); ++i)
				missingKeys += i->_key + ' ';

			return parserError(Common::String::format("Unhandled property inside key '%s' (%s, %d items).", key->name.c_str(), missingKeys.c_str(), keyCount));
		}

	} else {
		if (!handleUnknownKey(key))
			return parserError("Unexpected key in the active scope ('" + key->name + "').");
		ignore = true;
	}

	// check if any of the parents must be ignored.
	// if a parent is ignored, all children are too.
	for (int i = _activeKey.size() - 1; i >= 0; --i) {
		if (_activeKey[i]->ignore)
			ignore = true;
	}

	if (ignore == false && keyCallback(key) == false) {
		// HACK:  People may be stupid and overlook the fact that
		// when keyCallback() fails, a parserError() must be set.
		// We set it manually in that case.
		if (_state != kParserError)
			parserError("Unhandled exception when parsing '" + key->name + "' key.");

		return false;
	}

	if (closed)
		return closeKey();

	return true;
}

bool XMLParser::parseKeyValue(String keyName) {
	assert(_activeKey.empty() == false);

	if (_activeKey.top()->values.contains(keyName))
		return false;

	_token.clear();
	char stringStart;

	if (_char == '"' || _char == '\'') {
		stringStart = _char;
		_char = _stream->readByte();

		while (_char && _char != stringStart) {
			_token += _char;
			_char = _stream->readByte();
		}

		if (_char == 0)
			return false;

		_char = _stream->readByte();

	} else if (!parseToken()) {
		return false;
	}

	_activeKey.top()->values[keyName] = _token;
	return true;
}

bool XMLParser::parseIntegerKey(const char *key, int count, ...) {
	bool result;
	va_list args;
	va_start(args, count);
	result = vparseIntegerKey(key, count, args);
	va_end(args);
	return result;
}

bool XMLParser::parseIntegerKey(const String &key, int count, ...) {
	bool result;
	va_list args;
	va_start(args, count);
	result = vparseIntegerKey(key.c_str(), count, args);
	va_end(args);
	return result;
}

bool XMLParser::vparseIntegerKey(const char *key, int count, va_list args) {
	char *parseEnd;
	int *num_ptr;

	while (count--) {
		while (isSpace(*key))
			key++;

		num_ptr = va_arg(args, int*);
		*num_ptr = strtol(key, &parseEnd, 10);

		key = parseEnd;

		while (isSpace(*key))
			key++;

		if (count && *key++ != ',')
			return false;
	}

	return (*key == 0);
}

bool XMLParser::closeKey() {
	bool ignore = false;
	bool result = true;

	for (int i = _activeKey.size() - 1; i >= 0; --i) {
		if (_activeKey[i]->ignore)
			ignore = true;
	}

	if (ignore == false)
		result = closedKeyCallback(_activeKey.top());

	freeNode(_activeKey.pop());

	return result;
}

bool XMLParser::parse() {
	if (_stream == nullptr)
		return false;

	// Make sure we are at the start of the stream.
	_stream->seek(0, SEEK_SET);

	if (_XMLkeys == nullptr)
		buildLayout();

	while (!_activeKey.empty())
		freeNode(_activeKey.pop());

	cleanup();

	bool activeClosure = false;
	bool activeHeader = false;
	bool selfClosure;

	_state = kParserNeedHeader;
	_activeKey.clear();

	_char = _stream->readByte();

	while (_char && _state != kParserError) {
		if (skipSpaces())
			continue;

		if (skipComments())
			continue;

		switch (_state) {
		case kParserNeedHeader:
		case kParserNeedKey:
			if (_char != '<') {
				if (_allowText) {
					Common::String text;
					do {
						text += _char;
						_char = _stream->readByte();
					} while (_char != '<' && _char);
					if (!_char) {
						parserError("Unexpected end of file.");
						break;
					}
					if (!textCallback(text)) {
						parserError("Failed to process text segment.");
						break;
					}
				} else {
					parserError("Parser expecting key start.");
					break;
				}
			}

			if ((_char = _stream->readByte()) == 0) {
				parserError("Unexpected end of file.");
				break;
			}

			if (_state == kParserNeedHeader) {
				if (_char != '?') {
					parserError("Expecting XML header.");
					break;
				}

				_char = _stream->readByte();
				activeHeader = true;
			} else if (_char == '/') {
				_char = _stream->readByte();
				activeClosure = true;
			} else if (_char == '?') {
				parserError("Unexpected header. There may only be one XML header per file.");
				break;
			}

			_state = kParserNeedKeyName;
			break;

		case kParserNeedKeyName:
			if (!parseToken()) {
				parserError("Invalid key name.");
				break;
			}

			if (activeClosure) {
				if (_activeKey.empty() || _token != _activeKey.top()->name) {
					parserError("Unexpected closure.");
					break;
				}
			} else {
				ParserNode *node = allocNode(); // new ParserNode;
				node->name = _token;
				node->ignore = false;
				node->header = activeHeader;
				node->depth = _activeKey.size();
				node->layout = nullptr;
				_activeKey.push(node);
			}

			_state = kParserNeedPropertyName;
			break;

		case kParserNeedPropertyName:
			if (activeClosure) {
				if (!closeKey()) {
					parserError("Missing data when closing key '" + _activeKey.top()->name + "'.");
					break;
				}

				activeClosure = false;

				if (_char != '>')
					parserError("Invalid syntax in key closure.");
				else
					_state = kParserNeedKey;

				_char = _stream->readByte();
				break;
			}

			selfClosure = false;

			if (_char == '/' || (_char == '?' && activeHeader)) {
				selfClosure = true;
				_char = _stream->readByte();
			}

			if (_char == '>') {
				if (activeHeader && !selfClosure) {
					parserError("XML Header must be self-closed.");
				} else if (parseActiveKey(selfClosure)) {
					_char = _stream->readByte();
					_state = kParserNeedKey;
				}

				activeHeader = false;
				break;
			}

			if (selfClosure)
				parserError("Expecting key closure after '/' symbol.");
			else if (!parseToken())
				parserError("Error when parsing key value.");
			else
				_state = kParserNeedPropertyOperator;

			break;

		case kParserNeedPropertyOperator:
			if (_char != '=')
				parserError("Syntax error after key name.");
			else
				_state = kParserNeedPropertyValue;

			_char = _stream->readByte();
			break;

		case kParserNeedPropertyValue:
			if (!parseKeyValue(_token))
				parserError("Invalid key value.");
			else
				_state = kParserNeedPropertyName;

			break;

		default:
			break;
		}
	}

	if (_state == kParserError)
		return false;

	if (_state != kParserNeedKey || !_activeKey.empty())
		return parserError("Unexpected end of file.");

	return true;
}

bool XMLParser::skipSpaces() {
	if (!isSpace(_char))
		return false;

	while (_char && isSpace(_char))
		_char = _stream->readByte();

	return true;
}

bool XMLParser::skipComments() {
	if (_char == '<') {
		_char = _stream->readByte();

		if (_char != '!') {
			_stream->seek(-1, SEEK_CUR);
			_char = '<';
			return false;
		}

		if (_stream->readByte() != '-' || _stream->readByte() != '-')
			return parserError("Malformed comment syntax.");

		_char = _stream->readByte();

		while (_char) {
			if (_char == '-') {
				if (_stream->readByte() == '-') {

					if (_stream->readByte() != '>')
						return parserError("Malformed comment (double-hyphen inside comment body).");

					_char = _stream->readByte();
					return true;
				}
			}

			_char = _stream->readByte();
		}

		return parserError("Comment has no closure.");
	}

	return false;
}

bool XMLParser::parseToken() {
	_token.clear();

	while (isValidNameChar(_char)) {
		_token += _char;
		_char = _stream->readByte();
	}

	return isSpace(_char) != 0 || _char == '>' || _char == '=' || _char == '/';
}

} // End of namespace Common
