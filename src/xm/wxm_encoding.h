///////////////////////////////////////////////////////////////////////////////
// vim:         ts=4 sw=4
// Name:        wxmedit/wxm_encoding.h
// Description: define the Encodings which are supported by wxMEdit
// Author:      wxmedit@gmail.com
// Licence:     GPL
///////////////////////////////////////////////////////////////////////////////

#ifndef _WXM_ENCODING_H_
#define _WXM_ENCODING_H_

#include "../wxmedit/ucs4_t.h"

#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include <unicode/ucnv.h>
#include <boost/utility.hpp>
#include <boost/array.hpp>
#include <string>
#include <vector>
#include <map>

namespace wxm
{

enum WXMEncodingType
{ etSingleByte, etDoubleByte, etUTF8, etUTF16LE, etUTF16BE, etUTF32LE, etUTF32BE, etWXDoubleByte };


struct WXMEncoding;
struct WXMEncodingCreator: private boost::noncopyable
{
	static WXMEncodingCreator& Instance()
	{
		static WXMEncodingCreator creator;
		return creator;
	}

	void InitEncodings()
	{
		if (!m_initialized)
			DoInit();
		m_initialized = true;
	}
	void FreeEncodings();

	WXMEncoding* CreateWxmEncoding(ssize_t idx);
	WXMEncoding* CreateWxmEncoding(wxFontEncoding enc);
	WXMEncoding* CreateWxmEncoding(const wxString &name);

	size_t GetEncodingsCount();
	wxString GetEncodingName(ssize_t idx);
	std::string GetEncodingInnerName(ssize_t idx);
	wxString GetEncodingDescription(ssize_t idx);
	wxString GetEncodingFontName(ssize_t idx);
	wxString EncodingToName(wxFontEncoding enc);
	wxFontEncoding NameToEncoding(const wxString &name);
	WXMEncoding* GetSystemEncoding();

	WXMEncodingType GetIdxEncType(ssize_t idx);

private:
	ssize_t AdjustIndex(ssize_t idx);
	wxFontEncoding IdxToEncoding(ssize_t idx)
	{
		return NameToEncoding(GetEncodingName(idx));
	}

	void DoInit();
	void AddEncoding(const std::string& encname, wxFontEncoding wxenc
		, WXMEncodingType entype=etSingleByte, const std::string& innername0=std::string());

	WXMEncodingCreator()
	: m_initialized(false), m_sysenc_idx(-1), m_sysenc(NULL)
	{
		DoInit();
		m_initialized = true;
	}

	bool m_initialized;
	ssize_t m_sysenc_idx;
	WXMEncoding *m_sysenc;

	typedef std::map<std::string, wxFontEncoding> WXEncMap;
	WXEncMap m_wxenc_map;

	std::vector<wxString> m_wxenc_list;

	typedef std::map<wxString, wxFontEncoding> WXNameEncMap;
	typedef std::map<wxFontEncoding, wxString> WXEncNameMap;
	typedef std::map<wxFontEncoding, WXMEncodingType> WXEncTypeMap;
	typedef std::map<wxFontEncoding, wxString> WXEncFontMap;
	typedef std::map<wxFontEncoding, wxString> WXEncDescMap;
	typedef std::map<wxFontEncoding, std::string> WXEncInnerNameMap;
	WXNameEncMap m_wxnameenc_map;
	WXEncNameMap m_wxencname_map;
	WXEncTypeMap m_wxenctype_map;
	WXEncFontMap m_wxencfont_map;
	WXEncDescMap m_wxencdesc_map;
	WXEncInnerNameMap m_wxencinnername_map;

	typedef std::map<ssize_t, WXMEncoding*> WXEncInstMap;
	WXEncInstMap m_inst_map;
};

struct WXMEncoding: private boost::noncopyable
{
protected:
	wxString m_name;
	std::string m_innername;
	wxString m_desc;
	wxString m_fontname;
	wxFontEncoding m_enc;
	WXMEncodingType m_type;
	ssize_t m_idx;

	virtual void Create(ssize_t idx);

protected:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	friend void WXMEncodingCreator::FreeEncodings();
	WXMEncoding(): m_idx(-1)
	{
	}
	virtual ~WXMEncoding()
	{
	}

public:
	// return the converted length of buf
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf) = 0;

	virtual ucs4_t MultiBytetoUCS4(wxByte* buf)
	{
		return 0xFFFD;
	}

	virtual bool IsLeadByte(wxByte byte)
	{
		return false;
	}

	wxString GetName() { return m_name; }
	wxString GetDescription() { return m_desc; }
	WXMEncodingType GetType() { return m_type; }
	wxString GetFontName() { return m_fontname; }
	wxFontEncoding GetEncoding() { return m_enc; }
};

struct WXMEncodingMultiByte: public WXMEncoding
{
	virtual void MultiByteInit() = 0;
	virtual ucs4_t MultiBytetoUCS4(wxByte* buf) = 0;
	virtual void Create(ssize_t idx);

protected:
	WXMEncodingMultiByte(){}
	~WXMEncodingMultiByte(){}
};

struct MBConverter
{
	virtual size_t MB2WC(UChar32& ch, const char* src, size_t src_len) = 0;
	virtual size_t WC2MB(char* dest, size_t dest_len, const UChar32& ch) = 0;

	virtual ~MBConverter() {}
};

struct ICUConverter: public MBConverter
{
	ICUConverter(const std::string& encname);
	~ICUConverter();

	virtual size_t MB2WC(UChar32& ch, const char* src, size_t src_len);
	virtual size_t WC2MB(char* dest, size_t dest_len, const UChar32& ch);
private:
	UConverter* m_ucnv;
};

struct WXConverter: public MBConverter
{
	WXConverter(const std::string& encname, wxFontEncoding enc);
	~WXConverter()
	{
		delete m_wxcnv; m_wxcnv = NULL;
	}

	virtual size_t MB2WC(UChar32& ch, const char* src, size_t src_len);
	virtual size_t WC2MB(char* dest, size_t dest_len, const UChar32& ch);
private:
	wxCSConv* m_wxcnv;
};

typedef boost::array<ucs4_t, 256> ByteUnicodeArr;
typedef std::map<ucs4_t, wxByte> UnicodeByteMap;

struct SingleByteEncodingTableFixer
{
	virtual ~SingleByteEncodingTableFixer() {}
	virtual void fix(ByteUnicodeArr& toutab, UnicodeByteMap& fromutab) {}
};

struct OEMTableFixer: public SingleByteEncodingTableFixer
{
	virtual void fix(ByteUnicodeArr& toutab, UnicodeByteMap& fromutab);
};

struct CP437TableFixer: public OEMTableFixer
{
	virtual void fix(ByteUnicodeArr& toutab, UnicodeByteMap& fromutab);
};

struct CP852TableFixer: public OEMTableFixer
{
	virtual void fix(ByteUnicodeArr& toutab, UnicodeByteMap& fromutab);
};

struct Windows874TableFixer: public SingleByteEncodingTableFixer
{
	virtual void fix(ByteUnicodeArr& toutab, UnicodeByteMap& fromutab);
};

struct WXMEncodingSingleByte: public WXMEncodingMultiByte
{
	virtual void MultiByteInit();
	virtual ucs4_t MultiBytetoUCS4(wxByte* buf);
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);
private:
	ICUConverter* m_icucnv;
	ByteUnicodeArr m_tounicode;
	UnicodeByteMap m_fromunicode;

	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	WXMEncodingSingleByte(): m_icucnv(NULL)
	{
	}
	~WXMEncodingSingleByte()
	{
		delete m_icucnv; m_icucnv = NULL;
	}

	SingleByteEncodingTableFixer* CreateSingleByteEncodingTableFixer();
};

struct WXMEncodingDoubleByte: public WXMEncodingMultiByte
{
	enum SpecialValueType{ svtInvaliad=0, svtNotCached=0xFF};

	virtual void MultiByteInit();
	virtual bool IsLeadByte(wxByte byte);
	virtual ucs4_t MultiBytetoUCS4(wxByte* buf);
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);

protected:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	virtual void InitMBConverter()
	{
		m_mbcnv = new ICUConverter(m_innername);
	}
	WXMEncodingDoubleByte(): m_mbcnv(NULL)
	{
	}
	~WXMEncodingDoubleByte()
	{
		delete m_mbcnv; m_mbcnv = NULL;
	}

	MBConverter* m_mbcnv;

private:
	enum LeadByteType{ lbUnset=0, lbLeadByte, lbNotLeadByte=0xFF};

	boost::array<wxByte, 256> m_leadbyte_tab;

	boost::array<ucs4_t, 256> m_b2u_tab;
	std::map<wxByte, boost::array<ucs4_t, 256> > m_db2u_tab;

	boost::array<wxWord, 0x10000> m_bmp2mb_tab;
	std::map<ucs4_t, wxWord> m_nonbmp2mb_map;

};

struct WXMEncodingWXDoubleByte: public WXMEncodingDoubleByte
{
private:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	virtual void Create(ssize_t idx)
	{
		WXMEncodingDoubleByte::Create(idx);

		m_type = etDoubleByte;
	}
	~WXMEncodingWXDoubleByte(){}

	virtual void InitMBConverter()
	{
		m_mbcnv = new WXConverter(m_innername, m_enc);
	}
};

struct WXMEncodingUTF8: public WXMEncoding
{
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);
private:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	WXMEncodingUTF8(){}
	~WXMEncodingUTF8(){}
};

struct WXMEncodingUTF16LE: public WXMEncoding
{
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);
private:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	WXMEncodingUTF16LE(){}
	~WXMEncodingUTF16LE(){}
};

struct WXMEncodingUTF16BE: public WXMEncoding
{
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);
private:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	WXMEncodingUTF16BE(){}
	~WXMEncodingUTF16BE(){}
};

struct WXMEncodingUTF32LE: public WXMEncoding
{
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);
private:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	WXMEncodingUTF32LE(){}
	~WXMEncodingUTF32LE(){}
};

struct WXMEncodingUTF32BE: public WXMEncoding
{
	virtual size_t UCS4toMultiByte(ucs4_t ucs4, wxByte* buf);
private:
	friend WXMEncoding* WXMEncodingCreator::CreateWxmEncoding(ssize_t idx);
	WXMEncodingUTF32BE(){}
	~WXMEncodingUTF32BE(){}
};

size_t UCS4toUTF16LE_U10000(ucs4_t ucs4, wxByte* buf);

};// namespace wxm

#endif