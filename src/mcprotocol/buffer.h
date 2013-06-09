// TODO:
// Решить, использовать ли обобщённые функции чтения, или оставить их только для буфера.
// Пока что только для буфера.
// Does buffer need "current" reading? Removing it will make things simpler.

#ifndef _MC_PROTOCOL_BUFFER_H_ // новая запись по неймспейсам
#define _MC_PROTOCOL_BUFFER_H_

//#include <vector>
//#include "mcprotocol_base.h"
#include <boost/endian/conversion.hpp>
#include "types.h"

namespace MC
{
namespace Protocol
{



class Buffer
{
public:
	Buffer();
	Buffer(size_t _startSize);

	// Считывает переменную (не массив) размером больше 1 байта
	//template<class T> size_t readMBVar(T& _dst, size_t _offset = npos);

	size_t skipBytes(size_t _nBytes, size_t _offset);

	size_t readInt8			(int8_t&	_dst, size_t _offset = npos); // Если npos, то читает по текущему сдвигу.
	size_t readBool			(bool&		_dst, size_t _offset = npos);
	size_t readInt16		(int16_t&	_dst, size_t _offset = npos);
	size_t readInt32		(int32_t&	_dst, size_t _offset = npos);
	size_t readInt64		(int64_t&	_dst, size_t _offset = npos);
	size_t readFloat		(float&		_dst, size_t _offset = npos);
	size_t readDouble		(double&	_dst, size_t _offset = npos);
	size_t readString16		(String16&	_dst, size_t _offset = npos);
	size_t readByteArray	(ByteArray&	_dst, size_t _size, size_t _offset = npos);

	size_t writeInt8		(int8_t		_src, size_t _offset = npos); // Если npos, то пишет по текущему сдвигу.
	size_t writeBool		(bool		_src, size_t _offset = npos);
	size_t writeInt16		(int16_t	_src, size_t _offset = npos);
	size_t writeInt32		(int32_t	_src, size_t _offset = npos);
	size_t writeInt64		(int64_t	_src, size_t _offset = npos);
	size_t writeFloat		(float		_src, size_t _offset = npos);
	size_t writeDouble		(double		_src, size_t _offset = npos);
	size_t writeString16	(String16	_src, size_t _offset = npos);
	size_t writeByteArray	(ByteArray	_src, size_t _offset = npos);

	static const size_t npos = -1;
	class Exception_NotEnoughDataToRead {};

private:
	std::vector<uint8_t> _pf_buffer;
	size_t _pf_readOffset;
	size_t _pf_writeOffset;

	// методы
	inline void _pm_checkIfEnoughBytesToRead(int _neededNBytes, size_t _offset);
	inline bool _pm_checkIfEnoughBytesToRead_noEx(int _neededNBytes, size_t _offset);
	// Checks if offset equals npos (this means user is reading current variable, not
	// from specified position), and fixes offset if needed.
	inline bool _pm_checkAndFixNposOffset(size_t& _offset, size_t _newVal);
	inline bool _pm_checkAndFixNposOffset(size_t _offset);
};



void Buffer::_pm_checkIfEnoughBytesToRead(int _neededNBytes, size_t _offset)
{
	if(_pf_buffer.size() - _offset < _neededNBytes)
		throw Exception_NotEnoughDataToRead();
}

bool Buffer::_pm_checkIfEnoughBytesToRead_noEx(int _neededNBytes, size_t _offset)
{
	//if(_pf_buffer.size() - _offset < _neededNBytes)
		//throw Exception_NotEnoughDataToRead();
	return _pf_buffer.size() - _offset >= _neededNBytes;
}

bool Buffer::_pm_checkAndFixNposOffset(size_t& _offset, size_t _newVal)
{
	if(_offset == npos)
	{
		_offset = _newVal;
		return true;
	}
	else
		return false;
}

bool Buffer::_pm_checkAndFixNposOffset(size_t _offset)
{
	if(_offset != npos)
		_pf_readOffset = _offset;
}


//template<class T>
//size_t Buffer::readMBVar(T& _dst, size_t _offset)
//{
//	if(_offset == npos)
//		_offset = _pf_readOffset;
//
//	_pm_checkIfEnoughBytesToRead(sizeof(_dst), _offset);
//
//	memcpy(&_dst, _pf_buffer.data() + _offset, sizeof(_dst));
//	boost::endian::big_to_native(_dst);
//
//	if(_offset == _pf_readOffset)
//		_pf_readOffset += sizeof(_dst);
//
//	_offset += sizeof(_dst);
//
//	return _offset;
//}


} // namespace Protocol
} // namespace MC

#endif // _MC_PROTOCOL_BUFFER_H_