#include "SipMethods.h"
#include <boost/format.hpp>
#include <string>
#include <regex>
#include <iostream>
#include <sstream>
using namespace std;
unordered_map<std::string, ESipMethod > SipMessage::m_map_string_sip_methods;
unordered_map<ESipMethod, std::string> SipMessage::m_map_sipmethods_string;
SipRequest SipRequest::operator= (const SipResponse& rhs) 
{
	SipRequest* res = new SipRequest();
	res->getfrom(rhs);
	return *(res);
}
void SipRequest::getfrom(const SipResponse& response)
{
	setTo(response.getToName(), response.getToURL(), response.getToTag());
	setFrom(response.getFromName(), response.getFromURL(), response.getFromTag());
	setVia(response.getVia());
	setSequence(response.getSequence());
	setCallId(response.getCallId());
	setSDP(response.getSDP());

}
SipMessage::SipMessage()
{
	SipMessage::m_map_sipmethods_string[UNKNOWN] = "UNKNOWN";
	SipMessage::m_map_sipmethods_string[INVITE] = "INVITE";
	SipMessage::m_map_sipmethods_string[RINGNG] = "RINGNG";
	SipMessage::m_map_sipmethods_string[ACK] = "ACK";
	SipMessage::m_map_sipmethods_string[BYE] = "BYE";
	SipMessage::m_map_sipmethods_string[OK] = "200 OK";
	SipMessage::m_map_string_sip_methods["UNKNOWN"] = UNKNOWN;
	SipMessage::m_map_string_sip_methods["INVITE"] = INVITE;
	SipMessage::m_map_string_sip_methods["RINGNG"] = RINGNG;
	SipMessage::m_map_string_sip_methods["ACK"] = ACK;
	SipMessage::m_map_string_sip_methods["BYE"] = BYE;
	SipMessage::m_map_string_sip_methods["200 OK"] = OK;
}
	bool SipMessage::operator==(const SipMessage& rhs)
	{
		bool res =  m_type == rhs.m_type &&  m_to == rhs.m_to &&  m_to_URI == rhs.m_to_URI &&  m_to_tag == rhs.m_to_tag;
		res = res &&  m_from == rhs.m_from &&  m_from_URI == rhs.m_from_URI &&  m_from_tag == rhs.m_from_tag &&  m_via == rhs.m_via;
		res = res &&  m_call_id == rhs.m_call_id &&  m_sequence == rhs.m_sequence;
		res = res &&  m_sdp_len == rhs.m_sdp_len &&  m_addititonal_headers == rhs.m_addititonal_headers &&  m_content_type == rhs.m_content_type;
		return res;
	}
	SipRequest::SipRequest() 
	{
		m_type = UNKNOWN;
	}
	
	std::string SipRequest::toString()
	{
		string header = "";

		if (m_type == INVITE || m_type == ACK || m_type == BYE)

		{
			header = SipMessage::m_map_sipmethods_string[m_type] + ' ' + (m_to_URI);
			header = header + " SIP\/2.0\n"; 
		}
		else 
		{
			header = "SIP\/2.0 ";
			header =  header + SipMessage::m_map_sipmethods_string[m_type] + '\n';
		}

		boost::format fmt
			= boost::format{ "Via:%1%\n" \
				"From:%2%<%3%>%4%\n" \
				"To:%5%<%6%>%7%\n" \
				"Call-ID:%8%\n" \
				"CSeq:%9%\n" \
			//   "Contact: sip:sipp@{local_ip}:{local_port}\n" \ #TODO
			
			"%10%" \
			
			"Content-Length: %11%\n" \
			"\n" \
			"%12%"}% m_via% m_from% m_from_URI% m_from_tag% m_to% m_to_URI% m_to_tag% m_call_id% m_sequence%  m_content_type% m_sdp_len% m_sdp;
		return header  + fmt.str();
	}
	SipResponse::SipResponse(const std::string& str)
	{
		fromString(str);
	}
	bool SipResponse::parse(const string& regex_message, list<std::string*> vect_strings, const string& str)
	{
		std::regex message(regex_message);
		//std::cout <<"\n\nMESSAGE__\n\n" <<regex_message << "\n\nMESSAGE__\n\n" << str << endl;
		std::smatch matched;
		if (regex_match(str, matched, message))
		{
			int k = 1;
			for (auto i: vect_strings)
			{
				*(i) = matched[k];
				++k;
			}
			return true;
		}
		else
		{
			cout << "\n\nSipResponse couldn`t match anything\n\n" << endl;
			return false;
		}
	}

	bool SipResponse::parse_header(const std::string& str)
	{
		bool res;
		string type;
		string regex_message = "";
		list<string*> list_of_variables;
		if ((str.find(SipMessage::m_map_sipmethods_string[INVITE])) != string::npos || (str.find(SipMessage::m_map_sipmethods_string[ACK])) != string::npos || (str.find(SipMessage::m_map_sipmethods_string[BYE]) != string::npos))
		{
			regex_message = "(.*?) (.*?) SIP\/2.0\\s*";
			list_of_variables.push_back(&type);
			list_of_variables.push_back(&m_to_URI);
		}
		else
		{
			regex_message = "SIP\/2.0 (.*?)\\s*";
			list_of_variables.push_back(&type);
			 
		}
		res = parse(regex_message, list_of_variables, str);
		m_type = SipMessage::m_map_string_sip_methods[*list_of_variables.front()];
		return res;
	}

	bool SipResponse::fromString(const std::string& str)
	{
		std::istringstream sstream(str);
		std::string line;
		string regex_message = "(?s).*";
		getline(sstream, line);
		if (!parse_header(line))
		{
			return false;
		}
		bool is_sdp = false;
		while (!sstream.eof())
		{

			getline(sstream, line);
			if (is_sdp)
			{
				m_sdp = m_sdp + line;
				continue;
			}
			list<string*> list_of_variables;
			regex_message = "(?s).*";
			if (line.rfind("To", 0) == 0) 
			{
				regex_message = "To:(.*?)<(.*?)>(.*?)\\s*";
				list_of_variables.push_back(&m_to);
				list_of_variables.push_back(&m_to_URI);
				list_of_variables.push_back(&m_to_tag);
			}
			else if (line.rfind("From", 0) == 0)
			{
				regex_message = "From:(.*?)<(.*?)>(.*?)\\s*";
				list_of_variables.push_back(&m_from);
				list_of_variables.push_back(&m_from_URI);
				list_of_variables.push_back(&m_from_tag);
			}
			else if (line.rfind("Via", 0) == 0)
			{
				regex_message = "Via:(.*?)\\s*";
				list_of_variables.push_back(&m_via);
			}
			else if (line.rfind("CSeq:", 0) == 0)
			{
				regex_message = "CSeq:(.*?)\\s*";
				list_of_variables.push_back(&m_sequence);
			}
			else if (line.rfind("Call-ID", 0) == 0)
			{
				regex_message = "Call-ID:(.*?)\\s*";
				list_of_variables.push_back(&m_call_id);
			}
			else if (line.rfind("Content-Length:", 0) == 0)
			{
				regex_message = "Content-Length: (.*?)\\s*";
				list_of_variables.push_back(&m_sdp_len);
				is_sdp = true;
			}
			else 
			{
				continue;
			}
			if (!parse(regex_message, list_of_variables, line))
				return false;
		}
		return true;
	}
	ESipMethod SipResponse::type() const { return m_type; }

	void SipRequest::setType(ESipMethod m)
	{
		m_type = m;
	};
	void SipRequest::setUserAgent(const std::string& useragent) {};

	void SipRequest::setFrom(const std::string& from_name, const std::string& from_url, const std::string& from_tag) 
	{
		m_from = from_name;
		m_from_tag = from_tag;
		m_from_URI = from_url;
	};
	void SipRequest::setTo(const std::string& to_name, const std::string& to_url, const std::string& to_tag) 
	{
		m_to = to_name;
		m_to_tag = to_tag;
		m_to_URI = to_url;
	};
	void SipRequest::setSDP(const std::string& sdp) 
	{
		m_sdp = sdp;
		m_sdp_len = to_string(sdp.size());
	} 
	void SipRequest::setContentType(const std::string& content_type) 
	{
		m_content_type = content_type.size();
	};
	void SipRequest::setCallId(const std::string& id) 
	{
		m_call_id = id;
	}
	void SipRequest::setVia(const std::string& via)
	{
		m_via = via;
	}
	void SipRequest::setSequence(const std::string& seq) 
	{
		m_sequence = seq;
	}
	void SipRequest::setAddititonalHeaders(const std::string& headers)
	{
		m_addititonal_headers = headers;
	}
	std::string SipResponse::getFromName() const
	{
		return m_from;
	}
	std::string SipResponse::getFromURL() const
	{
		return m_from_URI;
	}
	std::string SipResponse::getFromTag() const
	{
		return m_from_tag;
	}
	std::string SipResponse::getToName() const
	{
		return m_to;
	}
	std::string SipResponse::getToURL() const
	{
		return m_to_URI;
	}
	std::string SipResponse::getToTag() const
	{
		return m_to_tag;
	}
	std::string SipResponse::getSDP() const
	{
		return m_sdp;
	}
	std::string SipResponse::getContentType() const
	{
		return m_content_type;
	};
	std::string SipResponse::getCallId() const
	{
		return m_call_id;
	}
	std::string SipResponse::getVia() const
	{
		return m_via;
	}
	std::string SipResponse::getSequence() const
	{
		return m_sequence;
	}
	std::string SipResponse::getAddititonalHeaders() const
	{
		return m_addititonal_headers;
	}


	/*
bool res = false;
string regex_message = "Via:(.*?)\n.*?From:(.*?)<(.*?)>(.*?)\nTo:(.*?)<(.*?)>(.*?)\n.*?Call-ID: (.*?)\nCSeq: (.*?)\n"\
		"(.*?).*?" \
		"Content-Length: (.*?)\n" \
		"\n" \
		"(.*?)";
list<string*> vec_of_variables = { &m_via, &m_from, &m_from_URI, &m_from_tag, &m_to, &m_to_URI, &m_to_tag, &m_call_id, &m_sequence, &m_addititonal_headers, &m_sdp_len, &m_sdp };
string type;
if ((str.find(SipMessage::m_map_sipmethods_string[INVITE])) != string::npos || (str.find(SipMessage::m_map_sipmethods_string[ACK])) != string::npos || (str.find(SipMessage::m_map_sipmethods_string[BYE]) != string::npos))
{
	regex_message = "(.*?) (.*?) SIP\/2.0\n" + regex_message;

	vec_of_variables.push_front(&m_to_URI);
	vec_of_variables.push_front(&type);
}
else
{
	regex_message = "SIP\/2.0 (.*?)\n" + regex_message;
	vec_of_variables.push_front(&type);
}
res =  parse(regex_message, vec_of_variables, str);
m_type = SipMessage::m_map_string_sip_methods[type];
return res;*/