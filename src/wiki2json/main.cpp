#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <array>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/regex.hpp>
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
	std::string id;
	std::string name;
	std::string direction;
	std::string comment;
	std::string wikiLink;
	std::vector<FieldInfo> fields;

	void clear()
	{
		id.clear();
		name.clear();
		direction.clear();
		comment.clear();
		wikiLink.clear();
		fields.clear();
	}
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
	/****
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
	|}
	****/

	// Clearing PacketInfo
	_dst.clear();

	// Splitting block into string vector
	std::vector<std::string> tokenVec;
	boost::split_regex(tokenVec, _block, boost::regex("(\\n\\|)"));

	// Splitting first line to get code, name etc
	std::vector<std::string> bufVec;
	boost::split(bufVec, tokenVec[0], boost::is_any_of("\n"));
	bufVec.erase(std::remove_if(bufVec.begin(), bufVec.end(), 
		[](const std::string& _s){ return _s.empty(); }), bufVec.end());
	bufVec.pop_back(); // removing unneeded line

	// ---- Storing packet comment ----
	for(int i = 2; i < bufVec.size(); ++i)
	{
		bufVec[i] += '\n';
		_dst.comment += bufVec[i];
	}

	// ---- Storing packet direction ----
	std::copy(bufVec[1].begin() + 2, bufVec[1].end() - 2, std::back_inserter(_dst.direction));

	// ---- Getting and storing packet name and code ----

	// === Keep Alive (0x00) ===
	boost::regex re_nameAndId("=== (.+(?= \\()) \\((.+)\\) ===");
	boost::smatch match;

	boost::regex_match(bufVec[0], match, re_nameAndId);
	_dst.name = match[1];
	_dst.id = match[2];

	// ---- Wiki link ----
	_dst.wikiLink = "www.wiki.vg/Protocol#" + _dst.id;

	// ---- Fields ----

	// Getting number of fields
	// Fields start from tokenVec[9]; tokenVec[8] can be parsed to get number of fields.

	int nFields;

	// Bad fragment, needs to be reviewed.
	bufVec.clear();
	// N in rowspan="N" is equal to number of fields; 1 if absent
	boost::find_all_regex(bufVec, tokenVec[8], boost::regex("rowspan=\"(\\d+)\"")); 
	if(bufVec.empty())
		nFields = 1;
	else
		nFields = strtol(bufVec[0].c_str() + 9, NULL, 10);

	// Getting and fixing fields
	FieldInfo fi;

	for(int i = 9; i < nFields * 5 + 9; i += 5)
	{
		fi.name    = boost::trim_copy(tokenVec[i]);
		fi.type    = boost::trim_copy(tokenVec[i + 1]);
		fi.example = boost::trim_copy(tokenVec[i + 2]);
		fi.comment = boost::trim_copy(tokenVec[i + 3]);

		boost::erase_all(fi.example, "<code>");
		boost::erase_all(fi.example, "</code>");
		boost::erase_all(fi.comment, "<code>");
		boost::erase_all(fi.comment, "</code>");

		boost::to_lower(fi.type);

		if(fi.type.empty())
			fi.type = "special";
		else // premature optimization?
		{
			boost::replace_first(fi.type, "[[slot_data|slot]]", "slot");
			boost::replace_first(fi.type, "[[entities#entity_metadata_format|metadata]]", "metadata");
			boost::replace_first(fi.type, "[[object data]]", "object data");

			// boost::replace_all_regex() is better
			//boost::replace_regex(fi.type, boost::regex("^array of (.*?)s$"), "array:$1");
			boost::smatch m;
			// . is bad, needs replacing with more exact expressions
			if( boost::regex_match(fi.type, m, boost::regex("array of (.*?)s?")) ||
				boost::regex_match(fi.type, m, boost::regex("(.*?) array")))
				fi.type = "array:" + m[1];
			//std::string fmt = "array:$1";
			//fi.type = boost::regex_replace(fi.type, boost::regex("array of (.*)s"), fmt);
		}
		
		
		
		

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
		_ostr	<< "Packet name:      [" << pi.name << "]" << std::endl
				<< "Packet id:        [" << pi.id << "] (0x" << std::hex << pi.id << ")" << std::dec << std::endl
				<< "Packet direction: [" << pi.direction << "]" << std::endl
				<< "Wiki link:        [" << pi.wikiLink << "]" << std::endl << std::endl
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
	_packet.put("id", _src.id);
	_packet.put("name", _src.name);
	_packet.put("direction", _src.direction);
	_packet.put("comment", _src.comment);
	_packet.put("wikilink", _src.wikiLink);

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
		packetsArray.push_back(std::make_pair("", packetJson));
	}

	_packets.add_child("packets", packetsArray);
}


bool CheckType(const FieldInfo& _fi)
{
	// byte short int long float double string Byte array Metadata Object data slot
	// BAD! Construction on every call
	static std::vector<std::string> allowedTypes;
	static bool typesFilled = false;
	if(!typesFilled)
	{
		allowedTypes.push_back("boolean");
		allowedTypes.push_back("byte");
		allowedTypes.push_back("unsigned byte");
		allowedTypes.push_back("short");
		allowedTypes.push_back("unsigned short");
		allowedTypes.push_back("int");
		allowedTypes.push_back("long");
		allowedTypes.push_back("float");
		allowedTypes.push_back("double");
		allowedTypes.push_back("string");
		//allowedTypes.push_back("byte array");
		allowedTypes.push_back("metadata");
		allowedTypes.push_back("object data");
		allowedTypes.push_back("slot");

		typesFilled = true;
	}

	// format is array:type
	bool isArray = (_fi.type.find("array") != std::string::npos);

	//return std::search(allowedTypes.begin(), allowedTypes.end(), _fi.type.begin(), _fi.type.end()) != allowedTypes.end();
	return std::find(allowedTypes.begin(), allowedTypes.end(), 
		isArray ? _fi.type.c_str() + 6 : _fi.type) != allowedTypes.end();
}


// ---- Reading from JSON ----
void ReadJsonField();


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

	BOOST_FOREACH(auto &pk, packetsInfo)
		BOOST_FOREACH(auto &fld, pk.fields)
			if(!CheckType(fld))
			{
				std::cout	<< pk.id << " " << pk.name << "\n"
							<< fld.name << ": type is [" << fld.type << "]" << std::endl << std::endl;
			}

	system("pause");
	return 0;

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