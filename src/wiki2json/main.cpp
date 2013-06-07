#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <array>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
//#include <jsoncpp/json.h>

// [begin, end)
struct TokenOffsets
{
	size_t begin;
	size_t end;
};

struct FieldInfo
{
	std::string type;
	std::string name;
	std::string example;
	std::string comment;
};

struct PacketInfo
{
	std::string packetComment;
	std::string packetName;
	std::string packetDirection;

	int packetId;
	std::vector<FieldInfo> fields;
};

void TokenOffsetsToStr(const std::string& _src, std::string& _dst, const TokenOffsets& _toffs)
{
	_dst.clear();
	if(_toffs.begin != _toffs.end)
		std::copy(_src.begin() + _toffs.begin, _src.begin() + _toffs.end, std::back_inserter(_dst));
}

size_t FindTextBetweenFragments(const std::string& _src, const std::string& _before, const std::string& _after, 
	 TokenOffsets& _offs, size_t _offset, bool _throwExIfNpos = false)
{
	size_t beforeOff = _src.find(_before, _offset);
	if(beforeOff == std::string::npos)
		return beforeOff;

	beforeOff += _before.size();

	size_t afterOff = _src.find(_after, beforeOff);
	if(afterOff == std::string::npos)
		return afterOff;

	//_dst.clear();
	//std::copy(_src.begin() + beforeOff, _src.begin() + afterOff, std::back_inserter(_dst));
	_offs.begin = beforeOff;
	_offs.end   = afterOff;

	return afterOff + _after.size();
}

size_t GetPacketBlock(const std::string& _src, std::string& _dst, size_t _offset)
{
	static const std::string blockStart = "<p><br />\n<span id=";
	static const std::string blockEnd   = "</table>";

	TokenOffsets offs;
	_offset = FindTextBetweenFragments(_src, blockStart, blockEnd, offs, _offset);
	_dst.clear();
	std::copy(_src.begin() + offs.begin, _src.begin() + offs.end, std::back_inserter(_dst));

	return _offset;
}


void ParsePacketBlock(const std::string& _block, PacketInfo& _dst)
{
	/*
	<span id="0x00"></span>
</p>
<h3> <span class="mw-headline" id="Keep_Alive_.280x00.29"> Keep Alive (0x00) </span></h3>
<p><i>Two-Way</i>
</p><p>The server will frequently send out a keep-alive, each containing a random ID. The client must respond with the same packet.
The Beta server will disconnect a client if it doesn't receive at least one packet before 1200 in-game ticks, and the Beta client will time out the connection under the same conditions. The client may send packets with Keep-alive ID=0.
</p>
<table class="wikitable">

<tr>
<td> Packet ID
</td>
<td> Field Name
</td>
<td> Field Type
</td>
<td> Example
</td>
<td> Notes
</td></tr>
<tr>
<td> 0x00
</td>
<td> Keep-alive ID
</td>
<td> int
</td>
<td> <code>957759560</code>
</td>
<td> Server-generated random id
</td></tr>
<tr>
<td> Total Size:
</td>
<td colspan="4"> 5 bytes
</td></tr></table>
<p><br />
	*/

	size_t offset = 0;
	TokenOffsets toffs;
	int nFields;

	// Packet name
	// "> Keep Alive00)
	offset = FindTextBetweenFragments(_block, "\"> ", " (0x", toffs, offset);
	TokenOffsetsToStr(_block, _dst.packetName, toffs);

	// Packet direction
	//<p><i>Two-Way</i>
	offset = FindTextBetweenFragments(_block, "<p><i>", "</i>", toffs, offset);
	TokenOffsetsToStr(_block, _dst.packetDirection, toffs);

	// Packet comment
	//</p><p>The server blah blah</p>
	offset = FindTextBetweenFragments(_block, "</p><p>", "</p>", toffs, offset);
	TokenOffsetsToStr(_block, _dst.packetComment, toffs);

	// </td></tr>\n<tr>
	// This combination indicates start of needed data block.
	offset = _block.find("</td></tr>\n<tr>", offset);

	// Trying to find rowspan, it means this packet has >1 fields.
	size_t rowspanOff = _block.find("rowspan=", offset);
	if(rowspanOff != std::string::npos)
	{
		std::string nFieldsBuf;
		TokenOffsets tf;
		offset = FindTextBetweenFragments(_block, "\"", "\"", tf, rowspanOff);
		TokenOffsetsToStr(_block, nFieldsBuf, tf);

		nFields = boost::lexical_cast<int>(nFieldsBuf);
	}
	else
	{
		nFields = 1;
		offset += 16; // </td></tr>\n<tr>
	}

	//std::cout << "nFields: " << nFields << std::endl << "offset: " << std::cout << offset << std::endl;


	// Packet id
	// > 0x00\n</td>
	std::string idbuf;
	offset = FindTextBetweenFragments(_block, "> 0x", "\n</td>", toffs, offset);
	TokenOffsetsToStr(_block, idbuf, toffs);
	_dst.packetId = strtol(idbuf.c_str(), NULL, 16);

	// Further data is: (name, type, example, comment) * nFields
	// <td> Keep-alive ID
	// </td>
	// <td> int
	// </td>
	// <td> <code>957759560</code>
	// </td>
	// <td> Server-generated random id
	// </td></tr>

	FieldInfo fi;

	for(int i = 0; i < nFields; ++i)
	{
		// Name (needs fixing later)
		offset = FindTextBetweenFragments(_block, "<td> ", "\n</td>", toffs, offset);
		TokenOffsetsToStr(_block, fi.name, toffs);
		// Type
		offset = FindTextBetweenFragments(_block, "<td> ", "\n</td>", toffs, offset);
		TokenOffsetsToStr(_block, fi.type, toffs);
		// Skipping example
		offset = _block.find("</td>", offset);
		// Comment
		offset = FindTextBetweenFragments(_block, "<td> ", "\n</td>", toffs, offset);
		TokenOffsetsToStr(_block, fi.comment, toffs);

		boost::erase_all(fi.comment, "<code>");
		boost::erase_all(fi.comment, "</code>");

		_dst.fields.push_back(fi);
	}
}


// ---- Raw wiki page parsing ----

size_t GetPacketBlock_raw(const std::string& _src, std::string& _dst, size_t _offset)
{
	static const std::string blockStart = "{{anchor|0x";
	static const std::string blockEnd   = "|}";

	TokenOffsets offs;
	_offset = FindTextBetweenFragments(_src, blockStart, blockEnd, offs, _offset);
	if(_offset == std::string::npos)
		return _offset;

	offs.begin += 5;

	_dst.clear();
	std::copy(_src.begin() + offs.begin, _src.begin() + offs.end, std::back_inserter(_dst));

	return _offset;
}

void ParsePacketBlock_raw(const std::string& _block, PacketInfo& _dst)
{
	/*{{anchor|0x00}}
	=== Keep Alive (0x00) ===
	''Two-Way''

	The server will frequently send out a keep-alive, each containing a random ID. The client must respond with the same packet.
	The Beta server will disconnect a client if it doesn't receive at least one packet before 1200 in-game ticks, and the Beta client will time out the connection under the same conditions. The client may send packets with Keep-alive ID=0.

	{| class="wikitable"
	|-
	| Packet ID
	| Field Name
	| Field Type
	| Example
	| Notes
	|-
	| 0x00
	| Keep-alive ID
	| int
	| <code>957759560</code>
	| Server-generated random id
	|-
	| Total Size:
	| colspan="4" | 5 bytes
	|}*/

	size_t offset = 0;
	TokenOffsets toffs;
	int nFields;

	_dst.fields.clear();

	// Packet name
	// === Keep Alive (0x00) ===
	offset = FindTextBetweenFragments(_block, "=== ", " (0", toffs, offset);
	TokenOffsetsToStr(_block, _dst.packetName, toffs);

	// Packet code
	// x00) ===
	std::string idbuf;
	offset = FindTextBetweenFragments(_block, "x", ")", toffs, offset);
	TokenOffsetsToStr(_block, idbuf, toffs);
	_dst.packetId = strtol(idbuf.c_str(), NULL, 16);

	// Packet direction
	// ''Two-Way''
	offset = FindTextBetweenFragments(_block, "''", "''", toffs, offset);
	TokenOffsetsToStr(_block, _dst.packetDirection, toffs);

	// Packet comment
	// \n\nThe server blah blah\n{|
	offset = FindTextBetweenFragments(_block, "\n\n", "\n{|", toffs, offset);
	TokenOffsetsToStr(_block, _dst.packetComment, toffs);

	// Notes\n|-
	// This combination indicates start of needed data block.
	offset = _block.find("Notes\n|-", offset) + 8;

	// Trying to find rowspan, it means this packet has >1 fields.
	size_t rowspanOff = _block.find("rowspan=", offset);
	if(rowspanOff != std::string::npos)
	{
		std::string nFieldsBuf;
		TokenOffsets tf;
		FindTextBetweenFragments(_block, "\"", "\"", tf, rowspanOff);
		TokenOffsetsToStr(_block, nFieldsBuf, tf);

		nFields = boost::lexical_cast<int>(nFieldsBuf);
	}
	else
		nFields = 1;

	// Moving to fields
	offset = _block.find("| 0x") + 6;

	// Further data is: (name, type, example, comment) * nFields
	// | Keep-alive ID
	// | int
	// | <code>957759560</code>
	// | Server-generated random id
	// |-

	//FieldInfo fi;
	//size_t offset2;

	for(int i = 0; i < nFields; ++i)
	{
		FieldInfo fi; // may be slower, but clears all fields

		// Name
		offset = FindTextBetweenFragments(_block, "| ", "\n", toffs, offset);
		TokenOffsetsToStr(_block, fi.name, toffs);

		// Type
		offset = FindTextBetweenFragments(_block, "|", "\n", toffs, offset);
		if(toffs.begin != toffs.end)
			TokenOffsetsToStr(_block, fi.type, toffs);
		else
			// Type is not specified in wiki.
			// Needs looking into wiki and fixing manually.
			fi.type = "Special"; 

		// Example
		offset = FindTextBetweenFragments(_block, "|", "\n", toffs, offset);
		if(toffs.begin != toffs.end)
			TokenOffsetsToStr(_block, fi.example, toffs);

		// Comment
		offset = FindTextBetweenFragments(_block, "|", "\n", toffs, offset);
		if(toffs.begin != toffs.end)
			TokenOffsetsToStr(_block, fi.comment, toffs);

		// Fixing fields
		boost::erase_all(fi.comment, "<code>");
		boost::erase_all(fi.comment, "</code>");
		boost::erase_all(fi.example, "<code>");
		boost::erase_all(fi.example, "</code>");
		boost::trim(fi.name);
		boost::trim(fi.type);
		boost::trim(fi.example);
		boost::trim(fi.comment);

		_dst.fields.push_back(fi);
	}
}


void ParseRawWikiPage(const std::string& _src, std::vector<PacketInfo>& _dst)
{
	PacketInfo pi;
	size_t offset = 0;
	std::string block;

	_dst.clear();

	while((offset = GetPacketBlock_raw(_src, block, offset)) != std::string::npos)
	{
		ParsePacketBlock_raw(block, pi);
		_dst.push_back(pi);
	}
}


void DebugPacketsOutput(const std::vector<PacketInfo>& _src, std::ostream& _ostr = std::cout)
{
	std::ofstream ofs_packets("packets_out.txt");
	BOOST_FOREACH(auto &pi, _src)
	{
		_ostr	<< "Packet name:      [" << pi.packetName << "]" << std::endl
				<< "Packet id:        [" << pi.packetId << "] (0x" << std::hex << pi.packetId << ")" << std::dec << std::endl
				<< "Packet direction: [" << pi.packetDirection << "]" << std::endl << std::endl
				<< "Fields (" << pi.fields.size() << "):" << std::endl;

		BOOST_FOREACH(auto &i, pi.fields)
			_ostr << "[" << i.type << "] [" << i.name << "]" << std::endl;

		_ostr << std::endl << std::endl;
	}

	ofs_packets.flush();
}


// ---- Writing to JSON ----

void PacketToJson(const PacketInfo& _src, boost::property_tree::ptree& _packet)
{
	_packet.clear();

	// Data
	_packet.put("id", _src.packetId);
	_packet.put("name", _src.packetName);
	_packet.put("direction", _src.packetDirection);
	_packet.put("comment", _src.packetComment);

	// Fields
	boost::property_tree::ptree array;

	for(int i = 0; i < _src.fields.size(); ++i)
	{
		boost::property_tree::ptree field;
		field.put("type", _src.fields[i].type);
		field.put("name", _src.fields[i].name);
		field.put("example", _src.fields[i].example);
		field.put("comment", _src.fields[i].comment);
		
		array.push_back(std::make_pair("", field));
	}

	_packet.add_child("fields", array);
}

void PacketsToJson(const std::vector<PacketInfo>& _src, boost::property_tree::ptree& _packets)
{
	_packets.clear();

	boost::property_tree::ptree packetsArray;

	boost::property_tree::ptree packetJson;
	BOOST_FOREACH(auto &packetInfo, _src)
	{
		
		PacketToJson(packetInfo, packetJson);

		//// Data
		//packetJson.put("id", packetInfo.packetId);
		//packetJson.put("name", packetInfo.packetName);
		//packetJson.put("direction", packetInfo.packetDirection);
		//packetJson.put("comment", packetInfo.packetComment);

		//// Fields
		//boost::property_tree::ptree array;

		//for(int i = 0; i < packetInfo.fields.size(); ++i)
		//{
		//	boost::property_tree::ptree field;
		//	field.put("type", packetInfo.fields[i].type);
		//	field.put("name", packetInfo.fields[i].name);
		//	field.put("example", packetInfo.fields[i].example);
		//	field.put("comment", packetInfo.fields[i].comment);
		//
		//	array.push_back(std::make_pair("", field));
		//}

		//packetJson.add_child("fields", array);

		packetsArray.push_back(std::make_pair("", packetJson));
	}

	_packets.add_child("packets", packetsArray);
}



int main(int argc, char* argv[])
{
	std::string rawPage, destJson;

	if(argc != 1 && argc != 3)
	{
		std::string appName = argv[0];
		boost::replace_all(appName, "\\", "/");
		appName.erase(appName.begin(), appName.begin() + appName.rfind('/') + 1);
		boost::erase_last(appName, ".exe");

		std::cout << "Usage: " << appName << " src_raw_wiki_page dest_json_file" << std::endl;
		return 0;
	}

	else if(argc == 1)
	{
		std::cout << "Enter paths to source raw wiki page and destination JSON file." << std::endl;
		std::cin >> rawPage >> destJson;
	}

	else
	{
		rawPage  = argv[1];
		destJson = argv[2];
	}


	std::ifstream page(rawPage);
	if(!page)
	{
		std::cout << "Failed to open file." << std::endl;
		return 1;
	}

	std::string pagetext;
	std::getline(page, pagetext, '\0');
	page.close();

	std::string block;
	size_t offset = 0;
	std::vector<PacketInfo> packetsInfo;

	ParseRawWikiPage(pagetext, packetsInfo);

	// Debug output
	//std::ofstream ofs_packets("packets_out.txt");
	//DebugPacketsOutput(packetsInfo, ofs_packets);
	//ofs_packets.close();

	// ---- Saving to JSON ----
	boost::property_tree::ptree packetsJson;
	PacketsToJson(packetsInfo, packetsJson);

	boost::property_tree::write_json(destJson, packetsJson);
				
	return 0;
}