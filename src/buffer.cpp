#include "buffer.h"

namespace MC
{
namespace Protocol
{

size_t Buffer::skipBytes(size_t _nBytes, size_t _offset)
{
	_pm_checkIfEnoughBytesToRead(_nBytes, _offset);
	return _offset + _nBytes;
}


size_t Buffer::readInt8(int8_t&	_dst, size_t _offset)
{
	if(_offset == npos)
		_offset = _pf_readOffset;

	_pm_checkIfEnoughBytesToRead(1, _offset);

	_dst = _pf_buffer[_offset];

	if(_offset == _pf_readOffset)
		_pf_readOffset += sizeof(_dst);

	_offset += sizeof(_dst);

	return _offset;
}

size_t Buffer::readBool(bool& _dst, size_t _offset)
{
	return readInt8(reinterpret_cast<int8_t&>(_dst), _offset);
}

// int16 .. int64, да и float double можно зашаблонить
size_t Buffer::readInt16(int16_t& _dst, size_t _offset)
{
	if(_offset == npos)
		_offset = _pf_readOffset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _offset);

	//_dst = *reinterpret_cast<int16_t*>(_pf_buffer.data() + _offset);
	memcpy(&_dst, _pf_buffer.data() + _offset, sizeof(_dst));
	boost::endian::big_to_native(_dst);

	if(_offset == _pf_readOffset)
		_pf_readOffset += sizeof(_dst);

	_offset += sizeof(_dst);

	return _offset;
}

size_t Buffer::readInt32(int32_t& _dst, size_t _offset)
{
	if(_offset == npos)
		_offset = _pf_readOffset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _offset);

	memcpy(&_dst, _pf_buffer.data() + _offset, sizeof(_dst));
	boost::endian::big_to_native(_dst);

	if(_offset == _pf_readOffset)
		_pf_readOffset += sizeof(_dst);

	_offset += sizeof(_dst);

	return _offset;
}

size_t Buffer::readInt64(int64_t& _dst, size_t _offset)
{
	if(_offset == npos)
		_offset = _pf_readOffset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _offset);

	memcpy(&_dst, _pf_buffer.data() + _offset, sizeof(_dst));
	boost::endian::big_to_native(_dst);

	if(_offset == _pf_readOffset)
		_pf_readOffset += sizeof(_dst);

	_offset += sizeof(_dst);

	return _offset;
}

size_t Buffer::readFloat(float& _dst, size_t _offset)
{
	if(_offset == npos)
		_offset = _pf_readOffset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _offset);

	memcpy(&_dst, _pf_buffer.data() + _offset, sizeof(_dst));
	boost::endian::big_to_native(reinterpret_cast<int32_t&>(_dst));

	if(_offset == _pf_readOffset)
		_pf_readOffset += sizeof(_dst);

	_offset += sizeof(_dst);

	return _offset;
}

size_t Buffer::readDouble(double& _dst, size_t _offset)
{
	if(_offset == npos)
		_offset = _pf_readOffset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _offset);

	memcpy(&_dst, _pf_buffer.data() + _offset, sizeof(_dst));
	boost::endian::big_to_native(reinterpret_cast<int64_t&>(_dst));

	if(_offset == _pf_readOffset)
		_pf_readOffset += sizeof(_dst);

	_offset += sizeof(_dst);

	return _offset;
}

size_t Buffer::readString16(String16& _dst, size_t _offset)
{

	// If we have _offset == npos and leave it to readInt16,
	// it will modify _pf_readOffset, and if string is not
	// fully received, there will be problems.
	bool readingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_readOffset);

	int16_t strLen;
	_offset = readInt16(strLen, _offset);

	_dst.clear();
	_dst.reserve(strLen);

	int16_t ch;
	for(int i = 0; i < strLen; ++i)
	{
		_offset = readInt16(ch, _offset);
		_dst.push_back(ch);
	}

	// Fixing readOffset if we were reading current var.
	if(readingCurrent)
		_pf_readOffset = _offset;

	return _offset;
}

size_t Buffer::readByteArray(ByteArray& _dst, size_t _size, size_t _offset)
{
	bool readingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_readOffset);

	_pm_checkIfEnoughBytesToRead(_size, _offset);

	_dst.clear();
	_dst.reserve(_size);
	std::copy(_pf_buffer.begin() + _offset, _pf_buffer.begin() + _offset + _size, _dst.begin());

	if(readingCurrent)
		_pf_readOffset = _offset + _size;

	return _offset + _size;
}


size_t Buffer::writeInt8(int8_t _src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	if(writingCurrent)
		_pf_buffer.push_back(_src);
	else
		_pf_buffer[_offset] = _src;

	return _offset + 1;
}

size_t Buffer::writeBool(bool _src, size_t _offset)
{
	return writeInt8(static_cast<int8_t>(_src), _offset);
}

// Опять полные копии, надо пошаблонить
size_t Buffer::writeInt16(int16_t _src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	boost::endian::native_to_big(_src);
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_offset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_offset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _offset);

	return _offset + sizeof(_src);
}

size_t Buffer::writeInt32(int32_t _src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	boost::endian::native_to_big(_src);
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_offset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_offset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _offset);

	return _offset + sizeof(_src);
}

size_t Buffer::writeInt64(int64_t _src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	boost::endian::native_to_big(_src);
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_offset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_offset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _offset);

	return _offset + sizeof(_src);
}

size_t Buffer::writeFloat(float _src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	boost::endian::native_to_big(reinterpret_cast<uint32_t&>(_src));
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_offset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_offset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _offset);

	return _offset + sizeof(_src);
}

size_t Buffer::writeDouble(double _src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	boost::endian::native_to_big(reinterpret_cast<uint64_t&>(_src));
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_offset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_offset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _offset);

	return _offset + sizeof(_src);
}

size_t Buffer::writeString16(String16 _src, size_t _offset)
{
	_pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	_offset = writeInt16(_src.size(), _offset);
	for(int i = 0; i < _src.size(); ++i)
		_offset = writeInt16(_src[i], _offset);

	return _offset;
}

size_t Buffer::writeByteArray(ByteArray	_src, size_t _offset)
{
	bool writingCurrent = _pm_checkAndFixNposOffset(_offset, _pf_buffer.size());

	if(_offset + _src.size() > _pf_buffer.size())
		_pf_buffer.resize(_offset + _src.size());

	std::copy(_src.begin(), _src.end(), _pf_buffer.begin() + _offset);

	return _offset + _src.size();
}

} // namespace Protocol
} // namespace MC