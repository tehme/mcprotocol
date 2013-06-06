// Новая суть чтения.
// Если передаётся npos, то читать по readOffset.
// Если передаётся сдвиг, то запомнить его в readOffset, чтобы потом можно было делать по npos.

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
	//_pm_checkAndFixNposOffset(_offset, _pf_readOffset);
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(1, _pf_readOffset);

	_dst = _pf_buffer[_pf_readOffset];
	_pf_readOffset += sizeof(_dst);

	return _pf_readOffset;
}

size_t Buffer::readBool(bool& _dst, size_t _offset)
{
	return readInt8(reinterpret_cast<int8_t&>(_dst), _offset);
}

// int16 .. int64, да и float double можно зашаблонить
size_t Buffer::readInt16(int16_t& _dst, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _pf_readOffset);

	_dst = *reinterpret_cast<int16_t*>(_pf_buffer.data() + _pf_readOffset);
	boost::endian::big_to_native(_dst);

	_pf_readOffset += sizeof(_dst);
	return _pf_readOffset;
}

size_t Buffer::readInt32(int32_t& _dst, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _pf_readOffset);

	_dst = *reinterpret_cast<int32_t*>(_pf_buffer.data() + _pf_readOffset);
	boost::endian::big_to_native(_dst);

	_pf_readOffset += sizeof(_dst);
	return _pf_readOffset;
}

size_t Buffer::readInt64(int64_t& _dst, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _pf_readOffset);

	_dst = *reinterpret_cast<int64_t*>(_pf_buffer.data() + _pf_readOffset);
	boost::endian::big_to_native(_dst);

	_pf_readOffset += sizeof(_dst);
	return _pf_readOffset;
}

size_t Buffer::readFloat(float& _dst, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _pf_readOffset);

	_dst = *reinterpret_cast<float*>(_pf_buffer.data() + _pf_readOffset);
	boost::endian::big_to_native(reinterpret_cast<uint32_t&>(_dst));

	_pf_readOffset += sizeof(_dst);
	return _pf_readOffset;
}

size_t Buffer::readDouble(double& _dst, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _pf_readOffset);

	_dst = *reinterpret_cast<float*>(_pf_buffer.data() + _pf_readOffset);
	boost::endian::big_to_native(reinterpret_cast<uint64_t&>(_dst));

	_pf_readOffset += sizeof(_dst);
	return _pf_readOffset;
}

size_t Buffer::readString16(String16& _dst, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	int16_t strLen;

	_pm_checkIfEnoughBytesToRead(sizeof(strLen), _pf_readOffset);
	readInt16(strLen, _offset); // <-- readOffset += 2;

	// Штука спорной нужности, так как откатывать всё это придётся ловцу исключения.
	// Но на всякий случай сделаем так.
	if(!_pm_checkIfEnoughBytesToRead_noEx(strLen, _pf_readOffset))
	{
		_pf_readOffset -= sizeof(strLen);
		throw Exception_NotEnoughDataToRead();
	}

	int16_t ch;
	for(int i = 0; i < strLen; ++i)
	{
		readInt16(ch);
		_dst.push_back(ch);
	}

	return _pf_readOffset;

}

size_t Buffer::readByteArray(ByteArray& _dst, size_t _size, size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;

	_pm_checkIfEnoughBytesToRead(_size, _pf_readOffset);

	_dst.clear();
	_dst.reserve(_size);
	std::copy(_pf_buffer.begin() + _offset, _pf_buffer.begin() + _offset + _size, _dst.begin());

	_pf_readOffset += _size;

	return _pf_readOffset;
}


size_t Buffer::writeInt8(int8_t _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
	{
		_pf_writeOffset = _pf_buffer.size();
		_pf_buffer.resize(_pf_buffer.size() + 1);
	}

	_pf_buffer[_pf_writeOffset] = _src;

	_pf_writeOffset += 1;

	return _pf_writeOffset;
}

size_t Buffer::writeBool(bool _src, size_t _offset)
{
	return writeInt8(static_cast<int8_t>(_src), _offset);
}

// Опять полные копии, надо пошаблонить
size_t Buffer::writeInt16(int16_t _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	boost::endian::native_to_big(_src);
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_pf_writeOffset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_pf_writeOffset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _pf_writeOffset);

	return _pf_writeOffset += sizeof(_src);
}

size_t Buffer::writeInt32(int32_t _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	boost::endian::native_to_big(_src);
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_pf_writeOffset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_pf_writeOffset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _pf_writeOffset);

	return _pf_writeOffset += sizeof(_src);
}

size_t Buffer::writeInt64(int64_t _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	boost::endian::native_to_big(_src);
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_pf_writeOffset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_pf_writeOffset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _pf_writeOffset);

	return _pf_writeOffset += sizeof(_src);
}

size_t Buffer::writeFloat(float _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	boost::endian::native_to_big(reinterpret_cast<uint32_t&>(_src));
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_pf_writeOffset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_pf_writeOffset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _pf_writeOffset);

	return _pf_writeOffset += sizeof(_src);
}

size_t Buffer::writeDouble(double _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	boost::endian::native_to_big(reinterpret_cast<uint32_t&>(_src));
	uint8_t *pSrc = reinterpret_cast<uint8_t*>(&_src);

	// Топорное решение, поправить
	if(_pf_writeOffset + sizeof(_src) > _pf_buffer.size())
		_pf_buffer.resize(_pf_writeOffset + sizeof(_src));

	std::copy(pSrc, pSrc + sizeof(_src), _pf_buffer.begin() + _pf_writeOffset);

	return _pf_writeOffset += sizeof(_src);
}

size_t Buffer::writeString16(String16 _src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	writeInt16(_src.size(), _pf_writeOffset);
	for(int i = 0; i < _src.size(); ++i)
		writeInt16(_src[i], _offset);

	return _pf_writeOffset;
}

size_t Buffer::writeByteArray(ByteArray	_src, size_t _offset)
{
	if(_offset != npos)
		_pf_writeOffset = _offset;
	else
		_pf_writeOffset = _pf_buffer.size();

	if(_pf_writeOffset + _src.size() > _pf_buffer.size())
		_pf_buffer.resize(_pf_writeOffset + _src.size());

	std::copy(_src.begin(), _src.end(), _pf_buffer.begin() + _pf_writeOffset);

	return _pf_writeOffset += _src.size();
}

} // namespace Protocol
} // namespace MC