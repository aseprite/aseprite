/*
www.sourceforge.net/projects/tinyxml
Original code (2.0 and earlier )copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#ifndef TINYXML_INCLUDED
#define TINYXML_INCLUDED

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Help out windows:
#if defined( _DEBUG ) && !defined( DEBUG )
#define DEBUG
#endif

#ifdef TIXML_USE_STL
	#include <string>
 	#include <iostream>
	#include <sstream>
	#define TIXML_STRING		std::string
#else
	#include "tinystr.h"
	#define TIXML_STRING		TiXmlString
#endif

// Deprecated library function hell. Compilers want to use the
// new safe versions. This probably doesn't fully address the problem,
// but it gets closer. There are too many compilers for me to fully
// test. If you get compilation troubles, undefine TIXML_SAFE
#define TIXML_SAFE

#ifdef TIXML_SAFE
	#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		// Microsoft visual studio, version 2005 and higher.
		#define TIXML_SNPRINTF _snprintf_s
		#define TIXML_SNSCANF  _snscanf_s
		#define TIXML_SSCANF   sscanf_s
	#elif defined(_MSC_VER) && (_MSC_VER >= 1200 )
		// Microsoft visual studio, version 6 and higher.
		//#pragma message( "Using _sn* functions." )
		#define TIXML_SNPRINTF _snprintf
		#define TIXML_SNSCANF  _snscanf
		#define TIXML_SSCANF   sscanf
	#elif defined(__GNUC__) && (__GNUC__ >= 3 )
		// GCC version 3 and higher.s
		//#warning( "Using sn* functions." )
		#define TIXML_SNPRINTF snprintf
		#define TIXML_SNSCANF  snscanf
		#define TIXML_SSCANF   sscanf
	#else
		#define TIXML_SSCANF   sscanf
	#endif
#endif	

class TiXmlDocument;
class TiXmlElement;
class TiXmlComment;
class TiXmlUnknown;
class TiXmlAttribute;
class TiXmlText;
class TiXmlDeclaration;
class TiXmlParsingData;

const int TIXML_MAJOR_VERSION = 2;
const int TIXML_MINOR_VERSION = 5;
const int TIXML_PATCH_VERSION = 3;

/*	Internal structure for tracking location of items 
	in the XML file.
*/
struct TiXmlCursor
{
	TiXmlCursor()		{ Clear(); }
	void Clear()		{ row = col = -1; }

	int row;	// 0 based.
	int col;	// 0 based.
};


/**
	If you call the Accept() method, it requires being passed a TiXmlVisitor
	class to handle callbacks. For nodes that contain other nodes (Document, Element)
	you will get called with a VisitEnter/VisitExit pair. Nodes that are always leaves
	are simple called with Visit().

	If you return 'true' from a Visit method, recursive parsing will continue. If you return
	false, <b>no children of this node or its sibilings</b> will be Visited.

	All flavors of Visit methods have a default implementation that returns 'true' (continue 
	visiting). You need to only override methods that are interesting to you.

	Generally Accept() is called on the TiXmlDocument, although all nodes suppert Visiting.

	You should never change the document from a callback.

	@sa TiXmlNode::Accept()
*/
class TiXmlVisitor
{
public:
	virtual ~TiXmlVisitor() {}

	/// Visit a document.
	virtual bool VisitEnter( const TiXmlDocument& /*doc*/ )			{ return true; }
	/// Visit a document.
	virtual bool VisitExit( const TiXmlDocument& /*doc*/ )			{ return true; }

	/// Visit an element.
	virtual bool VisitEnter( const TiXmlElement& /*element*/, const TiXmlAttribute* /*firstAttribute*/ )	{ return true; }
	/// Visit an element.
	virtual bool VisitExit( const TiXmlElement& /*element*/ )		{ return true; }

	/// Visit a declaration
	virtual bool Visit( const TiXmlDeclaration& /*declaration*/ )	{ return true; }
	/// Visit a text node
	virtual bool Visit( const TiXmlText& /*text*/ )					{ return true; }
	/// Visit a comment node
	virtual bool Visit( const TiXmlComment& /*comment*/ )			{ return true; }
	/// Visit an unknow node
	virtual bool Visit( const TiXmlUnknown& /*unknown*/ )			{ return true; }
};

// Only used by Attribute::Query functions
enum 
{ 
	TIXML_SUCCESS,
	TIXML_NO_ATTRIBUTE,
	TIXML_WRONG_TYPE
};


// Used by the parsing routines.
enum TiXmlEncoding
{
	TIXML_ENCODING_UNKNOWN,
	TIXML_ENCODING_UTF8,
	TIXML_ENCODING_LEGACY
};

const TiXmlEncoding TIXML_DEFAULT_ENCODING = TIXML_ENCODING_UNKNOWN;

/** TiXmlBase is a base class for every class in TinyXml.
	It does little except to establish that TinyXml classes
	can be printed and provide some utility functions.

	In XML, the document and elements can contain
	other elements and other types of nodes.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration( leaf )

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributes (not on tree)
							Comment (leaf)
							Unknown (leaf)

	A Decleration contains: Attributes (not on tree)
	@endverbatim
*/
class TiXmlBase
{
	friend class TiXmlNode;
	friend class TiXmlElement;
	friend class TiXmlDocument;

public:
	TiXmlBase()	:	userData(0)		{}
	virtual ~TiXmlBase()			{}

	/**	All TinyXml classes can print themselves to a filestream
		or the string class (TiXmlString in non-STL mode, std::string
		in STL mode.) Either or both cfile and str can be null.
		
		This is a formatted print, and will insert 
		tabs and newlines.
		
		(For an unformatted stream, use the << operator.)
	*/
	virtual void Print( FILE* cfile, int depth ) const = 0;

	/**	The world does not agree on whether white space should be kept or
		not. In order to make everyone happy, these global, static functions
		are provided to set whether or not TinyXml will condense all white space
		into a single space or not. The default is to condense. Note changing this
		value is not thread safe.
	*/
	static void SetCondenseWhiteSpace( bool condense )		{ condenseWhiteSpace = condense; }

	/// Return the current white space setting.
	static bool IsWhiteSpaceCondensed()						{ return condenseWhiteSpace; }

	/** Return the position, in the original source file, of this node or attribute.
		The row and column are 1-based. (That is the first row and first column is
		1,1). If the returns values are 0 or less, then the parser does not have
		a row and column value.

		Generally, the row and column value will be set when the TiXmlDocument::Load(),
		TiXmlDocument::LoadFile(), or any TiXmlNode::Parse() is called. It will NOT be set
		when the DOM was created from operator>>.

		The values reflect the initial load. Once the DOM is modified programmatically
		(by adding or changing nodes and attributes) the new values will NOT update to
		reflect changes in the document.

		There is a minor performance cost to computing the row and column. Computation
		can be disabled if TiXmlDocument::SetTabSize() is called with 0 as the value.

		@sa TiXmlDocument::SetTabSize()
	*/
	int Row() const			{ return location.row + 1; }
	int Column() const		{ return location.col + 1; }	///< See Row()

	void  SetUserData( void* user )			{ userData = user; }	///< Set a pointer to arbitrary user data.
	void* GetUserData()						{ return userData; }	///< Get a pointer to arbitrary user data.
	const void* GetUserData() const 		{ return userData; }	///< Get a pointer to arbitrary user data.

	// Table that returs, for a given lead byte, the total number of bytes
	// in the UTF-8 sequence.
	static const int utf8ByteTable[256];

	virtual const char* Parse(	const char* p, 
								TiXmlParsingData* data, 
								TiXmlEncoding encoding /*= TIXML_ENCODING_UNKNOWN */ ) = 0;

	/** Expands entities in a string. Note this should not contian the tag's '<', '>', etc, 
		or they will be transformed into entities!
	*/
	static void EncodeString( const TIXML_STRING& str, TIXML_STRING* out );

	enum
	{
		TIXML_NO_ERROR = 0,
		TIXML_ERROR,
		TIXML_ERROR_OPENING_FILE,
		TIXML_ERROR_OUT_OF_MEMORY,
		TIXML_ERROR_PARSING_ELEMENT,
		TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME,
		TIXML_ERROR_READING_ELEMENT_VALUE,
		TIXML_ERROR_READING_ATTRIBUTES,
		TIXML_ERROR_PARSING_EMPTY,
		TIXML_ERROR_READING_END_TAG,
		TIXML_ERROR_PARSING_UNKNOWN,
		TIXML_ERROR_PARSING_COMMENT,
		TIXML_ERROR_PARSING_DECLARATION,
		TIXML_ERROR_DOCUMENT_EMPTY,
		TIXML_ERROR_EMBEDDED_NULL,
		TIXML_ERROR_PARSING_CDATA,
		TIXML_ERROR_DOCUMENT_TOP_ONLY,

		TIXML_ERROR_STRING_COUNT
	};

protected:

	static const char* SkipWhiteSpace( const char*, TiXmlEncoding encoding );
	inline static bool IsWhiteSpace( char c )		
	{ 
		return ( isspace( (unsigned char) c ) || c == '\n' || c == '\r' ); 
	}
	inline static bool IsWhiteSpace( int c )
	{
		if ( c < 256 )
			return IsWhiteSpace( (char) c );
		return false;	// Again, only truly correct for English/Latin...but usually works.
	}

	#ifdef TIXML_USE_STL
	static bool	StreamWhiteSpace( std::istream * in, TIXML_STRING * tag );
	static bool StreamTo( std::istream * in, int character, TIXML_STRING * tag );
	#endif

	/*	Reads an XML name into the string provided. Returns
		a pointer just past the last character of the name,
		or 0 if the function has an error.
	*/
	static const char* ReadName( const char* p, TIXML_STRING* name, TiXmlEncoding encoding );

	/*	Reads text. Returns a pointer past the given end tag.
		Wickedly complex options, but it keeps the (sensitive) code in one place.
	*/
	static const char* ReadText(	const char* in,				// where to start
									TIXML_STRING* text,			// the string read
									bool ignoreWhiteSpace,		// whether to keep the white space
									const char* endTag,			// what ends this text
									bool ignoreCase,			// whether to ignore case in the end tag
									TiXmlEncoding encoding );	// the current encoding

	// If an entity has been found, transform it into a character.
	static const char* GetEntity( const char* in, char* value, int* length, TiXmlEncoding encoding );

	// Get a character, while interpreting entities.
	// The length can be from 0 to 4 bytes.
	inline static const char* GetChar( const char* p, char* _value, int* length, TiXmlEncoding encoding )
	{
		assert( p );
		if ( encoding == TIXML_ENCODING_UTF8 )
		{
			*length = utf8ByteTable[ *((const unsigned char*)p) ];
			assert( *length >= 0 && *length < 5 );
		}
		else
		{
			*length = 1;
		}

		if ( *length == 1 )
		{
			if ( *p == '&' )
				return GetEntity( p, _value, length, encoding );
			*_value = *p;
			return p+1;
		}
		else if ( *length )
		{
			//strncpy( _value, p, *length );	// lots of compilers don't like this function (unsafe),
												// and the null terminator isn't needed
			for( int i=0; p[i] && i<*length; ++i ) {
				_value[i] = p[i];
			}
			return p + (*length);
		}
		else
		{
			// Not valid text.
			return 0;
		}
	}

	// Return true if the next characters in the stream are any of the endTag sequences.
	// Ignore case only works for english, and should only be relied on when comparing
	// to English words: StringEqual( p, "version", true ) is fine.
	static bool StringEqual(	const char* p,
								const char* endTag,
								bool ignoreCase,
								TiXmlEncoding encoding );

	static const char* errorString[ TIXML_ERROR_STRING_COUNT ];

	TiXmlCursor location;

    /// Field containing a generic user pointer
	void*			userData;
	
	// None of these methods are reliable for any language except English.
	// Good for approximation, not great for accuracy.
	static int IsAlpha( unsigned char anyByte, TiXmlEncoding encoding );
	static int IsAlphaNum( unsigned char anyByte, TiXmlEncoding encoding );
	inline static int ToLower( int v, TiXmlEncoding encoding )
	{
		if ( encoding == TIXML_ENCODING_UTF8 )
		{
			if ( v < 128 ) return tolower( v );
			return v;
		}
		else
		{
			return tolower( v );
		}
	}
	static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );

private:
	TiXmlBase( const TiXmlBase& );				// not implemented.
	void operator=( const TiXmlBase& base );	// not allowed.

	struct Entity
	{
		const char*     str;
		unsigned int	strLength;
		char		    chr;
	};
	enum
	{
		NUM_ENTITY = 5,
		MAX_ENTITY_LENGTH = 6

	};
	static Entity entity[ NUM_ENTITY ];
	static bool condenseWhiteSpace;
};


/** The parent class for everything in the Document Object Model.
	(Except for attributes).
	Nodes have siblings, a parent, and children. A node can be
	in a document, or stand on its own. The type of a TiXmlNode
	can be queried, and it can be cast to its more defined type.
*/
class TiXmlNode : public TiXmlBase
{
	friend class TiXmlDocument;
	friend class TiXmlElement;

public:
	#ifdef TIXML_USE_STL	

	    /** An input stream operator, for every class. Tolerant of newlines and
		    formatting, but doesn't expect them.
	    */
	    friend std::istream& operator >> (std::istream& in, TiXmlNode& base);

	    /** An output stream operator, for every class. Note that this outputs
		    without any newlines or formatting, as opposed to Print(), which
		    includes tabs and new lines.

		    The operator<< and operator>> are not completely symmetric. Writing
		    a node to a stream is very well defined. You'll get a nice stream
		    of output, without any extra whitespace or newlines.
		    
		    But reading is not as well defined. (As it always is.) If you create
		    a TiXmlElement (for example) and read that from an input stream,
		    the text needs to define an element or junk will result. This is
		    true of all input streams, but it's worth keeping in mind.

		    A TiXmlDocument will read nodes until it reads a root element, and
			all the children of that root element.
	    */	
	    friend std::ostream& operator<< (std::ostream& out, const TiXmlNode& base);

		/// Appends the XML node or attribute to a std::string.
		friend std::string& operator<< (std::string& out, const TiXmlNode& base );

	#endif

	/** The types of XML nodes supported by TinyXml. (All the
			unsupported types are picked up by UNKNOWN.)
	*/
	enum NodeType
	{
		DOCUMENT,
		ELEMENT,
		COMMENT,
		UNKNOWN,
		TEXT,
		DECLARATION,
		TYPECOUNT
	};

	virtual ~TiXmlNode();

	/** The meaning of 'value' changes for the specific type of
		TiXmlNode.
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim

		The subclasses will wrap this function.
	*/
	const char *Value() const { return value.c_str (); }

    #ifdef TIXML_USE_STL
	/** Return Value() as a std::string. If you only use STL,
	    this is more efficient than calling Value().
		Only available in STL mode.
	*/
	const std::string& ValueStr() const { return value; }
	#endif

	const TIXML_STRING& ValueTStr() const { return value; }

	/** Changes the value of the node. Defined as:
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim
	*/
	void SetValue(const char * _value) { value = _value;}

    #ifdef TIXML_USE_STL
	/// STL std::string form.
	void SetValue( const std::string& _value )	{ value = _value; }
	#endif

	/// Delete all the children of this node. Does not affect 'this'.
	void Clear();

	/// One step up the DOM.
	TiXmlNode* Parent()							{ return parent; }
	const TiXmlNode* Parent() const				{ return parent; }

	const TiXmlNode* FirstChild()	const		{ return firstChild; }	///< The first child of this node. Will be null if there are no children.
	TiXmlNode* FirstChild()						{ return firstChild; }
	const TiXmlNode* FirstChild( const char * value ) const;			///< The first child of this node with the matching 'value'. Will be null if none found.
	/// The first child of this node with the matching 'value'. Will be null if none found.
	TiXmlNode* FirstChild( const char * _value ) {
		// Call through to the const version - safe since nothing is changed. Exiting syntax: cast this to a const (always safe)
		// call the method, cast the return back to non-const.
		return const_cast< TiXmlNode* > ((const_cast< const TiXmlNode* >(this))->FirstChild( _value ));
	}
	const TiXmlNode* LastChild() const	{ return lastChild; }		/// The last child of this node. Will be null if there are no children.
	TiXmlNode* LastChild()	{ return lastChild; }
	
	const TiXmlNode* LastChild( const char * value ) const;			/// The last child of this node matching 'value'. Will be null if there are no children.
	TiXmlNode* LastChild( const char * _value ) {
		return const_cast< TiXmlNode* > ((const_cast< const TiXmlNode* >(this))->LastChild( _value ));
	}

    #ifdef TIXML_USE_STL
	const TiXmlNode* FirstChild( const std::string& _value ) const	{	return FirstChild (_value.c_str ());	}	///< STL std::string form.
	TiXmlNode* FirstChild( const std::string& _value )				{	return FirstChild (_value.c_str ());	}	///< STL std::string form.
	const TiXmlNode* LastChild( const std::string& _value ) const	{	return LastChild (_value.c_str ());	}	///< STL std::string form.
	TiXmlNode* LastChild( const std::string& _value )				{	return LastChild (_value.c_str ());	}	///< STL std::string form.
	#endif

	/** An alternate way to walk the children of a node.
		One way to iterate over nodes is:
		@verbatim
			for( child = parent->FirstChild(); child; child = child->NextSibling() )
		@endverbatim

		IterateChildren does the same thing with the syntax:
		@verbatim
			child = 0;
			while( child = parent->IterateChildren( child ) )
		@endverbatim

		IterateChildren takes the previous child as input and finds
		the next one. If the previous child is null, it returns the
		first. IterateChildren will return null when done.
	*/
	const TiXmlNode* IterateChildren( const TiXmlNode* previous ) const;
	TiXmlNode* IterateChildren( const TiXmlNode* previous ) {
		return const_cast< TiXmlNode* >( (const_cast< const TiXmlNode* >(this))->IterateChildren( previous ) );
	}

	/// This flavor of IterateChildren searches for children with a particular 'value'
	const TiXmlNode* IterateChildren( const char * value, const TiXmlNode* previous ) const;
	TiXmlNode* IterateChildren( const char * _value, const TiXmlNode* previous ) {
		return const_cast< TiXmlNode* >( (const_cast< const TiXmlNode* >(this))->IterateChildren( _value, previous ) );
	}

    #ifdef TIXML_USE_STL
	const TiXmlNode* IterateChildren( const std::string& _value, const TiXmlNode* previous ) const	{	return IterateChildren (_value.c_str (), previous);	}	///< STL std::string form.
	TiXmlNode* IterateChildren( const std::string& _value, const TiXmlNode* previous ) {	return IterateChildren (_value.c_str (), previous);	}	///< STL std::string form.
	#endif

	/** Add a new node related to this. Adds a child past the LastChild.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* InsertEndChild( const TiXmlNode& addThis );


	/** Add a new node related to this. Adds a child past the LastChild.

		NOTE: the node to be added is passed by pointer, and will be
		henceforth owned (and deleted) by tinyXml. This method is efficient
		and avoids an extra copy, but should be used with care as it
		uses a different memory model than the other insert functions.

		@sa InsertEndChild
	*/
	TiXmlNode* LinkEndChild( TiXmlNode* addThis );

	/** Add a new node related to this. Adds a child before the specified child.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* InsertBeforeChild( TiXmlNode* beforeThis, const TiXmlNode& addThis );

	/** Add a new node related to this. Adds a child after the specified child.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* InsertAfterChild(  TiXmlNode* afterThis, const TiXmlNode& addThis );

	/** Replace a child of this node.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* ReplaceChild( TiXmlNode* replaceThis, const TiXmlNode& withThis );

	/// Delete a child of this node.
	bool RemoveChild( TiXmlNode* removeThis );

	/// Navigate to a sibling node.
	const TiXmlNode* PreviousSibling() const			{ return prev; }
	TiXmlNode* PreviousSibling()						{ return prev; }

	/// Navigate to a sibling node.
	const TiXmlNode* PreviousSibling( const char * ) const;
	TiXmlNode* PreviousSibling( const char *_prev ) {
		return const_cast< TiXmlNode* >( (const_cast< const TiXmlNode* >(this))->PreviousSibling( _prev ) );
	}

    #ifdef TIXML_USE_STL
	const TiXmlNode* PreviousSibling( const std::string& _value ) const	{	return PreviousSibling (_value.c_str ());	}	///< STL std::string form.
	TiXmlNode* PreviousSibling( const std::string& _value ) 			{	return PreviousSibling (_value.c_str ());	}	///< STL std::string form.
	const TiXmlNode* NextSibling( const std::string& _value) const		{	return NextSibling (_value.c_str ());	}	///< STL std::string form.
	TiXmlNode* NextSibling( const std::string& _value) 					{	return NextSibling (_value.c_str ());	}	///< STL std::string form.
	#endif

	/// Navigate to a sibling node.
	const TiXmlNode* NextSibling() const				{ return next; }
	TiXmlNode* NextSibling()							{ return next; }

	/// Navigate to a sibling node with the given 'value'.
	const TiXmlNode* NextSibling( const char * ) const;
	TiXmlNode* NextSibling( const char* _next ) {
		return const_cast< TiXmlNode* >( (const_cast< const TiXmlNode* >(this))->NextSibling( _next ) );
	}

	/** Convenience function to get through elements.
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	const TiXmlElement* NextSiblingElement() const;
	TiXmlElement* NextSiblingElement() {
		return const_cast< TiXmlElement* >( (const_cast< const TiXmlNode* >(this))->NextSiblingElement() );
	}

	/** Convenience function to get through elements.
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	const TiXmlElement* NextSiblingElement( const char * ) const;
	TiXmlElement* NextSiblingElement( const char *_next ) {
		return const_cast< TiXmlElement* >( (const_cast< const TiXmlNode* >(this))->NextSiblingElement( _next ) );
	}

    #ifdef TIXML_USE_STL
	const TiXmlElement* NextSiblingElement( const std::string& _value) const	{	return NextSiblingElement (_value.c_str ());	}	///< STL std::string form.
	TiXmlElement* NextSiblingElement( const std::string& _value)				{	return NextSiblingElement (_value.c_str ());	}	///< STL std::string form.
	#endif

	/// Convenience function to get through elements.
	const TiXmlElement* FirstChildElement()	const;
	TiXmlElement* FirstChildElement() {
		return const_cast< TiXmlElement* >( (const_cast< const TiXmlNode* >(this))->FirstChildElement() );
	}

	/// Convenience function to get through elements.
	const TiXmlElement* FirstChildElement( const char * _value ) const;
	TiXmlElement* FirstChildElement( const char * _value ) {
		return const_cast< TiXmlElement* >( (const_cast< const TiXmlNode* >(this))->FirstChildElement( _value ) );
	}

    #ifdef TIXML_USE_STL
	const TiXmlElement* FirstChildElement( const std::string& _value ) const	{	return FirstChildElement (_value.c_str ());	}	///< STL std::string form.
	TiXmlElement* FirstChildElement( const std::string& _value )				{	return FirstChildElement (_value.c_str ());	}	///< STL std::string form.
	#endif

	/** Query the type (as an enumerated value, above) of this node.
		The possible types are: DOCUMENT, ELEMENT, COMMENT,
								UNKNOWN, TEXT, and DECLARATION.
	*/
	int Type() const	{ return type; }

	/** Return a pointer to the Document this node lives in.
		Returns null if not in a document.
	*/
	const TiXmlDocument* GetDocument() const;
	TiXmlDocument* GetDocument() {
		return const_cast< TiXmlDocument* >( (const_cast< const TiXmlNode* >(this))->GetDocument() );
	}

	/// Returns true if this node has no children.
	bool NoChildren() const						{ return !firstChild; }

	virtual const TiXmlDocument*    ToDocument()    const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlElement*     ToElement()     const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlComment*     ToComment()     const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlUnknown*     ToUnknown()     const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlText*        ToText()        const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlDeclaration* ToDeclaration() const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.

	virtual TiXmlDocument*          ToDocument()    { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlElement*           ToElement()	    { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlComment*           ToComment()     { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlUnknown*           ToUnknown()	    { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlText*	            ToText()        { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlDeclaration*       ToDeclaration() { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.

	/** Create an exact duplicate of this node and return it. The memory must be deleted
		by the caller. 
	*/
	virtual TiXmlNode* Clone() const = 0;

	/** Accept a hierchical visit the nodes in the TinyXML DOM. Every node in the 
		XML tree will be conditionally visited and the host will be called back
		via the TiXmlVisitor interface.

		This is essentially a SAX interface for TinyXML. (Note however it doesn't re-parse
		the XML for the callbacks, so the performance of TinyXML is unchanged by using this
		interface versus any other.)

		The interface has been based on ideas from:

		- http://www.saxproject.org/
		- http://c2.com/cgi/wiki?HierarchicalVisitorPattern 

		Which are both good references for "visiting".

		An example of using Accept():
		@verbatim
		TiXmlPrinter printer;
		tinyxmlDoc.Accept( &printer );
		const char* xmlcstr = printer.CStr();
		@endverbatim
	*/
	virtual bool Accept( TiXmlVisitor* visitor ) const = 0;

protected:
	TiXmlNode( NodeType _type );

	// Copy to the allocated object. Shared functionality between Clone, Copy constructor,
	// and the assignment operator.
	void CopyTo( TiXmlNode* target ) const;

	#ifdef TIXML_USE_STL
	    // The real work of the input operator.
	virtual void StreamIn( std::istream* in, TIXML_STRING* tag ) = 0;
	#endif

	// Figure out what is at *p, and parse it. Returns null if it is not an xml node.
	TiXmlNode* Identify( const char* start, TiXmlEncoding encoding );

	TiXmlNode*		parent;
	NodeType		type;

	TiXmlNode*		firstChild;
	TiXmlNode*		lastChild;

	TIXML_STRING	value;

	TiXmlNode*		prev;
	TiXmlNode*		next;

private:
	TiXmlNode( const TiXmlNode& );				// not implemented.
	void operator=( const TiXmlNode& base );	// not allowed.
};


/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not TiXmlNodes, since they are not
		  part of the tinyXML document object model. There are other
		  suggested ways to look at this problem.
*/
class TiXmlAttribute : public TiXmlBase
{
	friend class TiXmlAttributeSet;

public:
	/// Construct an empty attribute.
	TiXmlAttribute() : TiXmlBase()
	{
		document = 0;
		prev = next = 0;
	}

	#ifdef TIXML_USE_STL
	/// std::string constructor.
	TiXmlAttribute( const std::string& _name, const std::string& _value )
	{
		name = _name;
		value = _value;
		document = 0;
		prev = next = 0;
	}
	#endif

	/// Construct an attribute with a name and value.
	TiXmlAttribute( const char * _name, const char * _value )
	{
		name = _name;
		value = _value;
		document = 0;
		prev = next = 0;
	}

	const char*		Name()  const		{ return name.c_str(); }		///< Return the name of this attribute.
	const char*		Value() const		{ return value.c_str(); }		///< Return the value of this attribute.
	#ifdef TIXML_USE_STL
	const std::string& ValueStr() const	{ return value; }				///< Return the value of this attribute.
	#endif
	int				IntValue() const;									///< Return the value of this attribute, converted to an integer.
	double			DoubleValue() const;								///< Return the value of this attribute, converted to a double.

	// Get the tinyxml string representation
	const TIXML_STRING& NameTStr() const { return name; }

	/** QueryIntValue examines the value string. It is an alternative to the
		IntValue() method with richer error checking.
		If the value is an integer, it is stored in 'value' and 
		the call returns TIXML_SUCCESS. If it is not
		an integer, it returns TIXML_WRONG_TYPE.

		A specialized but useful call. Note that for success it returns 0,
		which is the opposite of almost all other TinyXml calls.
	*/
	int QueryIntValue( int* _value ) const;
	/// QueryDoubleValue examines the value string. See QueryIntValue().
	int QueryDoubleValue( double* _value ) const;

	void SetName( const char* _name )	{ name = _name; }				///< Set the name of this attribute.
	void SetValue( const char* _value )	{ value = _value; }				///< Set the value.

	void SetIntValue( int _value );										///< Set the value from an integer.
	void SetDoubleValue( double _value );								///< Set the value from a double.

    #ifdef TIXML_USE_STL
	/// STL std::string form.
	void SetName( const std::string& _name )	{ name = _name; }	
	/// STL std::string form.	
	void SetValue( const std::string& _value )	{ value = _value; }
	#endif

	/// Get the next sibling attribute in the DOM. Returns null at end.
	const TiXmlAttribute* Next() const;
	TiXmlAttribute* Next() {
		return const_cast< TiXmlAttribute* >( (const_cast< const TiXmlAttribute* >(this))->Next() ); 
	}

	/// Get the previous sibling attribute in the DOM. Returns null at beginning.
	const TiXmlAttribute* Previous() const;
	TiXmlAttribute* Previous() {
		return const_cast< TiXmlAttribute* >( (const_cast< const TiXmlAttribute* >(this))->Previous() ); 
	}

	bool operator==( const TiXmlAttribute& rhs ) const { return rhs.name == name; }
	bool operator<( const TiXmlAttribute& rhs )	 const { return name < rhs.name; }
	bool operator>( const TiXmlAttribute& rhs )  const { return name > rhs.name; }

	/*	Attribute parsing starts: first letter of the name
						 returns: the next char after the value end quote
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );

	// Prints this Attribute to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const {
		Print( cfile, depth, 0 );
	}
	void Print( FILE* cfile, int depth, TIXML_STRING* str ) const;

	// [internal use]
	// Set the document pointer so the attribute can report errors.
	void SetDocument( TiXmlDocument* doc )	{ document = doc; }

private:
	TiXmlAttribute( const TiXmlAttribute& );				// not implemented.
	void operator=( const TiXmlAttribute& base );	// not allowed.

	TiXmlDocument*	document;	// A pointer back to a document, for error reporting.
	TIXML_STRING name;
	TIXML_STRING value;
	TiXmlAttribute*	prev;
	TiXmlAttribute*	next;
};


/*	A class used to manage a group of attributes.
	It is only used internally, both by the ELEMENT and the DECLARATION.
	
	The set can be changed transparent to the Element and Declaration
	classes that use it, but NOT transparent to the Attribute
	which has to implement a next() and previous() method. Which makes
	it a bit problematic and prevents the use of STL.

	This version is implemented with circular lists because:
		- I like circular lists
		- it demonstrates some independence from the (typical) doubly linked list.
*/
class TiXmlAttributeSet
{
public:
	TiXmlAttributeSet();
	~TiXmlAttributeSet();

	void Add( TiXmlAttribute* attribute );
	void Remove( TiXmlAttribute* attribute );

	const TiXmlAttribute* First()	const	{ return ( sentinel.next == &sentinel ) ? 0 : sentinel.next; }
	TiXmlAttribute* First()					{ return ( sentinel.next == &sentinel ) ? 0 : sentinel.next; }
	const TiXmlAttribute* Last() const		{ return ( sentinel.prev == &sentinel ) ? 0 : sentinel.prev; }
	TiXmlAttribute* Last()					{ return ( sentinel.prev == &sentinel ) ? 0 : sentinel.prev; }

	const TiXmlAttribute*	Find( const char* _name ) const;
	TiXmlAttribute*	Find( const char* _name ) {
		return const_cast< TiXmlAttribute* >( (const_cast< const TiXmlAttributeSet* >(this))->Find( _name ) );
	}
	#ifdef TIXML_USE_STL
	const TiXmlAttribute*	Find( const std::string& _name ) const;
	TiXmlAttribute*	Find( const std::string& _name ) {
		return const_cast< TiXmlAttribute* >( (const_cast< const TiXmlAttributeSet* >(this))->Find( _name ) );
	}

	#endif

private:
	//*ME:	Because of hidden/disabled copy-construktor in TiXmlAttribute (sentinel-element),
	//*ME:	this class must be also use a hidden/disabled copy-constructor !!!
	TiXmlAttributeSet( const TiXmlAttributeSet& );	// not allowed
	void operator=( const TiXmlAttributeSet& );	// not allowed (as TiXmlAttribute)

	TiXmlAttribute sentinel;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class TiXmlElement : public TiXmlNode
{
public:
	/// Construct an element.
	TiXmlElement (const char * in_value);

	#ifdef TIXML_USE_STL
	/// std::string constructor.
	TiXmlElement( const std::string& _value );
	#endif

	TiXmlElement( const TiXmlElement& );

	void operator=( const TiXmlElement& base );

	virtual ~TiXmlElement();

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
	*/
	const char* Attribute( const char* name ) const;

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
		If the attribute exists and can be converted to an integer,
		the integer value will be put in the return 'i', if 'i'
		is non-null.
	*/
	const char* Attribute( const char* name, int* i ) const;

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
		If the attribute exists and can be converted to an double,
		the double value will be put in the return 'd', if 'd'
		is non-null.
	*/
	const char* Attribute( const char* name, double* d ) const;

	/** QueryIntAttribute examines the attribute - it is an alternative to the
		Attribute() method with richer error checking.
		If the attribute is an integer, it is stored in 'value' and 
		the call returns TIXML_SUCCESS. If it is not
		an integer, it returns TIXML_WRONG_TYPE. If the attribute
		does not exist, then TIXML_NO_ATTRIBUTE is returned.
	*/	
	int QueryIntAttribute( const char* name, int* _value ) const;
	/// QueryDoubleAttribute examines the attribute - see QueryIntAttribute().
	int QueryDoubleAttribute( const char* name, double* _value ) const;
	/// QueryFloatAttribute examines the attribute - see QueryIntAttribute().
	int QueryFloatAttribute( const char* name, float* _value ) const {
		double d;
		int result = QueryDoubleAttribute( name, &d );
		if ( result == TIXML_SUCCESS ) {
			*_value = (float)d;
		}
		return result;
	}

    #ifdef TIXML_USE_STL
	/** Template form of the attribute query which will try to read the
		attribute into the specified type. Very easy, very powerful, but
		be careful to make sure to call this with the correct type.
		
		NOTE: This method doesn't work correctly for 'string' types.

		@return TIXML_SUCCESS, TIXML_WRONG_TYPE, or TIXML_NO_ATTRIBUTE
	*/
	template< typename T > int QueryValueAttribute( const std::string& name, T* outValue ) const
	{
		const TiXmlAttribute* node = attributeSet.Find( name );
		if ( !node )
			return TIXML_NO_ATTRIBUTE;

		std::stringstream sstream( node->ValueStr() );
		sstream >> *outValue;
		if ( !sstream.fail() )
			return TIXML_SUCCESS;
		return TIXML_WRONG_TYPE;
	}
	/*
	 This is - in theory - a bug fix for "QueryValueAtribute returns truncated std::string"
	 but template specialization is hard to get working cross-compiler. Leaving the bug for now.
	 
	// The above will fail for std::string because the space character is used as a seperator.
	// Specialize for strings. Bug [ 1695429 ] QueryValueAtribute returns truncated std::string
	template<> int QueryValueAttribute( const std::string& name, std::string* outValue ) const
	{
		const TiXmlAttribute* node = attributeSet.Find( name );
		if ( !node )
			return TIXML_NO_ATTRIBUTE;
		*outValue = node->ValueStr();
		return TIXML_SUCCESS;
	}
	*/
	#endif

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const char* name, const char * _value );

    #ifdef TIXML_USE_STL
	const std::string* Attribute( const std::string& name ) const;
	const std::string* Attribute( const std::string& name, int* i ) const;
	const std::string* Attribute( const std::string& name, double* d ) const;
	int QueryIntAttribute( const std::string& name, int* _value ) const;
	int QueryDoubleAttribute( const std::string& name, double* _value ) const;

	/// STL std::string form.
	void SetAttribute( const std::string& name, const std::string& _value );
	///< STL std::string form.
	void SetAttribute( const std::string& name, int _value );
	#endif

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const char * name, int value );

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetDoubleAttribute( const char * name, double value );

	/** Deletes an attribute with the given name.
	*/
	void RemoveAttribute( const char * name );
    #ifdef TIXML_USE_STL
	void RemoveAttribute( const std::string& name )	{	RemoveAttribute (name.c_str ());	}	///< STL std::string form.
	#endif

	const TiXmlAttribute* FirstAttribute() const	{ return attributeSet.First(); }		///< Access the first attribute in this element.
	TiXmlAttribute* FirstAttribute() 				{ return attributeSet.First(); }
	const TiXmlAttribute* LastAttribute()	const 	{ return attributeSet.Last(); }		///< Access the last attribute in this element.
	TiXmlAttribute* LastAttribute()					{ return attributeSet.Last(); }

	/** Convenience function for easy access to the text inside an element. Although easy
		and concise, GetText() is limited compared to getting the TiXmlText child
		and accessing it directly.
	
		If the first child of 'this' is a TiXmlText, the GetText()
		returns the character string of the Text node, else null is returned.

		This is a convenient method for getting the text of simple contained text:
		@verbatim
		<foo>This is text</foo>
		const char* str = fooElement->GetText();
		@endverbatim

		'str' will be a pointer to "This is text". 
		
		Note that this function can be misleading. If the element foo was created from
		this XML:
		@verbatim
		<foo><b>This is text</b></foo> 
		@endverbatim

		then the value of str would be null. The first child node isn't a text node, it is
		another element. From this XML:
		@verbatim
		<foo>This is <b>text</b></foo> 
		@endverbatim
		GetText() will return "This is ".

		WARNING: GetText() accesses a child node - don't become confused with the 
				 similarly named TiXmlHandle::Text() and TiXmlNode::ToText() which are 
				 safe type casts on the referenced node.
	*/
	const char* GetText() const;

	/// Creates a new Element and returns it - the returned element is a copy.
	virtual TiXmlNode* Clone() const;
	// Print the Element to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;

	/*	Attribtue parsing starts: next char past '<'
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );

	virtual const TiXmlElement*     ToElement()     const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlElement*           ToElement()	          { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

	/** Walk the XML tree visiting this node and all of its children. 
	*/
	virtual bool Accept( TiXmlVisitor* visitor ) const;

protected:

	void CopyTo( TiXmlElement* target ) const;
	void ClearThis();	// like clear, but initializes 'this' object as well

	// Used to be public [internal use]
	#ifdef TIXML_USE_STL
	virtual void StreamIn( std::istream * in, TIXML_STRING * tag );
	#endif
	/*	[internal use]
		Reads the "value" of the element -- another element, or text.
		This should terminate with the current end tag.
	*/
	const char* ReadValue( const char* in, TiXmlParsingData* prevData, TiXmlEncoding encoding );

private:

	TiXmlAttributeSet attributeSet;
};


/**	An XML comment.
*/
class TiXmlComment : public TiXmlNode
{
public:
	/// Constructs an empty comment.
	TiXmlComment() : TiXmlNode( TiXmlNode::COMMENT ) {}
	/// Construct a comment from text.
	TiXmlComment( const char* _value ) : TiXmlNode( TiXmlNode::COMMENT ) {
		SetValue( _value );
	}
	TiXmlComment( const TiXmlComment& );
	void operator=( const TiXmlComment& base );

	virtual ~TiXmlComment()	{}

	/// Returns a copy of this Comment.
	virtual TiXmlNode* Clone() const;
	// Write this Comment to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;

	/*	Attribtue parsing starts: at the ! of the !--
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );

	virtual const TiXmlComment*  ToComment() const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlComment*  ToComment() { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

	/** Walk the XML tree visiting this node and all of its children. 
	*/
	virtual bool Accept( TiXmlVisitor* visitor ) const;

protected:
	void CopyTo( TiXmlComment* target ) const;

	// used to be public
	#ifdef TIXML_USE_STL
	virtual void StreamIn( std::istream * in, TIXML_STRING * tag );
	#endif
//	virtual void StreamOut( TIXML_OSTREAM * out ) const;

private:

};


/** XML text. A text node can have 2 ways to output the next. "normal" output 
	and CDATA. It will default to the mode it was parsed from the XML file and
	you generally want to leave it alone, but you can change the output mode with 
	SetCDATA() and query it with CDATA().
*/
class TiXmlText : public TiXmlNode
{
	friend class TiXmlElement;
public:
	/** Constructor for text element. By default, it is treated as 
		normal, encoded text. If you want it be output as a CDATA text
		element, set the parameter _cdata to 'true'
	*/
	TiXmlText (const char * initValue ) : TiXmlNode (TiXmlNode::TEXT)
	{
		SetValue( initValue );
		cdata = false;
	}
	virtual ~TiXmlText() {}

	#ifdef TIXML_USE_STL
	/// Constructor.
	TiXmlText( const std::string& initValue ) : TiXmlNode (TiXmlNode::TEXT)
	{
		SetValue( initValue );
		cdata = false;
	}
	#endif

	TiXmlText( const TiXmlText& copy ) : TiXmlNode( TiXmlNode::TEXT )	{ copy.CopyTo( this ); }
	void operator=( const TiXmlText& base )							 	{ base.CopyTo( this ); }

	// Write this text object to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;

	/// Queries whether this represents text using a CDATA section.
	bool CDATA() const				{ return cdata; }
	/// Turns on or off a CDATA representation of text.
	void SetCDATA( bool _cdata )	{ cdata = _cdata; }

	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );

	virtual const TiXmlText* ToText() const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlText*       ToText()       { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

	/** Walk the XML tree visiting this node and all of its children. 
	*/
	virtual bool Accept( TiXmlVisitor* content ) const;

protected :
	///  [internal use] Creates a new Element and returns it.
	virtual TiXmlNode* Clone() const;
	void CopyTo( TiXmlText* target ) const;

	bool Blank() const;	// returns true if all white space and new lines
	// [internal use]
	#ifdef TIXML_USE_STL
	virtual void StreamIn( std::istream * in, TIXML_STRING * tag );
	#endif

private:
	bool cdata;			// true if this should be input and output as a CDATA style text element
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXml will happily read or write files without a declaration,
	however. There are 3 possible attributes to the declaration:
	version, encoding, and standalone.

	Note: In this version of the code, the attributes are
	handled as special cases, not generic attributes, simply
	because there can only be at most 3 and they are always the same.
*/
class TiXmlDeclaration : public TiXmlNode
{
public:
	/// Construct an empty declaration.
	TiXmlDeclaration()   : TiXmlNode( TiXmlNode::DECLARATION ) {}

#ifdef TIXML_USE_STL
	/// Constructor.
	TiXmlDeclaration(	const std::string& _version,
						const std::string& _encoding,
						const std::string& _standalone );
#endif

	/// Construct.
	TiXmlDeclaration(	const char* _version,
						const char* _encoding,
						const char* _standalone );

	TiXmlDeclaration( const TiXmlDeclaration& copy );
	void operator=( const TiXmlDeclaration& copy );

	virtual ~TiXmlDeclaration()	{}

	/// Version. Will return an empty string if none was found.
	const char *Version() const			{ return version.c_str (); }
	/// Encoding. Will return an empty string if none was found.
	const char *Encoding() const		{ return encoding.c_str (); }
	/// Is this a standalone document?
	const char *Standalone() const		{ return standalone.c_str (); }

	/// Creates a copy of this Declaration and returns it.
	virtual TiXmlNode* Clone() const;
	// Print this declaration to a FILE stream.
	virtual void Print( FILE* cfile, int depth, TIXML_STRING* str ) const;
	virtual void Print( FILE* cfile, int depth ) const {
		Print( cfile, depth, 0 );
	}

	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );

	virtual const TiXmlDeclaration* ToDeclaration() const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlDeclaration*       ToDeclaration()       { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

	/** Walk the XML tree visiting this node and all of its children. 
	*/
	virtual bool Accept( TiXmlVisitor* visitor ) const;

protected:
	void CopyTo( TiXmlDeclaration* target ) const;
	// used to be public
	#ifdef TIXML_USE_STL
	virtual void StreamIn( std::istream * in, TIXML_STRING * tag );
	#endif

private:

	TIXML_STRING version;
	TIXML_STRING encoding;
	TIXML_STRING standalone;
};


/** Any tag that tinyXml doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.

	DTD tags get thrown into TiXmlUnknowns.
*/
class TiXmlUnknown : public TiXmlNode
{
public:
	TiXmlUnknown() : TiXmlNode( TiXmlNode::UNKNOWN )	{}
	virtual ~TiXmlUnknown() {}

	TiXmlUnknown( const TiXmlUnknown& copy ) : TiXmlNode( TiXmlNode::UNKNOWN )		{ copy.CopyTo( this ); }
	void operator=( const TiXmlUnknown& copy )										{ copy.CopyTo( this ); }

	/// Creates a copy of this Unknown and returns it.
	virtual TiXmlNode* Clone() const;
	// Print this Unknown to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;

	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );

	virtual const TiXmlUnknown*     ToUnknown()     const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlUnknown*           ToUnknown()	    { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

	/** Walk the XML tree visiting this node and all of its children. 
	*/
	virtual bool Accept( TiXmlVisitor* content ) const;

protected:
	void CopyTo( TiXmlUnknown* target ) const;

	#ifdef TIXML_USE_STL
	virtual void StreamIn( std::istream * in, TIXML_STRING * tag );
	#endif

private:

};


/** Always the top level node. A document binds together all the
	XML pieces. It can be saved, loaded, and printed to the screen.
	The 'value' of a document node is the xml file name.
*/
class TiXmlDocument : public TiXmlNode
{
public:
	/// Create an empty document, that has no name.
	TiXmlDocument();
	/// Create a document with a name. The name of the document is also the filename of the xml.
	TiXmlDocument( const char * documentName );

	#ifdef TIXML_USE_STL
	/// Constructor.
	TiXmlDocument( const std::string& documentName );
	#endif

	TiXmlDocument( const TiXmlDocument& copy );
	void operator=( const TiXmlDocument& copy );

	virtual ~TiXmlDocument() {}

	/** Load a file using the current document value.
		Returns true if successful. Will delete any existing
		document data before loading.
	*/
	bool LoadFile( TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );
	/// Save a file using the current document value. Returns true if successful.
	bool SaveFile() const;
	/// Load a file using the given filename. Returns true if successful.
	bool LoadFile( const char * filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );
	/// Save a file using the given filename. Returns true if successful.
	bool SaveFile( const char * filename ) const;
	/** Load a file using the given FILE*. Returns true if successful. Note that this method
		doesn't stream - the entire object pointed at by the FILE*
		will be interpreted as an XML file. TinyXML doesn't stream in XML from the current
		file location. Streaming may be added in the future.
	*/
	bool LoadFile( FILE*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );
	/// Save a file using the given FILE*. Returns true if successful.
	bool SaveFile( FILE* ) const;

	#ifdef TIXML_USE_STL
	bool LoadFile( const std::string& filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING )			///< STL std::string version.
	{
//		StringToBuffer f( filename );
//		return ( f.buffer && LoadFile( f.buffer, encoding ));
		return LoadFile( filename.c_str(), encoding );
	}
	bool SaveFile( const std::string& filename ) const		///< STL std::string version.
	{
//		StringToBuffer f( filename );
//		return ( f.buffer && SaveFile( f.buffer ));
		return SaveFile( filename.c_str() );
	}
	#endif

	/** Parse the given null terminated block of xml data. Passing in an encoding to this
		method (either TIXML_ENCODING_LEGACY or TIXML_ENCODING_UTF8 will force TinyXml
		to use that encoding, regardless of what TinyXml might otherwise try to detect.
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data = 0, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );

	/** Get the root element -- the only top level element -- of the document.
		In well formed XML, there should only be one. TinyXml is tolerant of
		multiple elements at the document level.
	*/
	const TiXmlElement* RootElement() const		{ return FirstChildElement(); }
	TiXmlElement* RootElement()					{ return FirstChildElement(); }

	/** If an error occurs, Error will be set to true. Also,
		- The ErrorId() will contain the integer identifier of the error (not generally useful)
		- The ErrorDesc() method will return the name of the error. (very useful)
		- The ErrorRow() and ErrorCol() will return the location of the error (if known)
	*/	
	bool Error() const						{ return error; }

	/// Contains a textual (english) description of the error if one occurs.
	const char * ErrorDesc() const	{ return errorDesc.c_str (); }

	/** Generally, you probably want the error string ( ErrorDesc() ). But if you
		prefer the ErrorId, this function will fetch it.
	*/
	int ErrorId()	const				{ return errorId; }

	/** Returns the location (if known) of the error. The first column is column 1, 
		and the first row is row 1. A value of 0 means the row and column wasn't applicable
		(memory errors, for example, have no row/column) or the parser lost the error. (An
		error in the error reporting, in that case.)

		@sa SetTabSize, Row, Column
	*/
	int ErrorRow() const	{ return errorLocation.row+1; }
	int ErrorCol() const	{ return errorLocation.col+1; }	///< The column where the error occured. See ErrorRow()

	/** SetTabSize() allows the error reporting functions (ErrorRow() and ErrorCol())
		to report the correct values for row and column. It does not change the output
		or input in any way.
		
		By calling this method, with a tab size
		greater than 0, the row and column of each node and attribute is stored
		when the file is loaded. Very useful for tracking the DOM back in to
		the source file.

		The tab size is required for calculating the location of nodes. If not
		set, the default of 4 is used. The tabsize is set per document. Setting
		the tabsize to 0 disables row/column tracking.

		Note that row and column tracking is not supported when using operator>>.

		The tab size needs to be enabled before the parse or load. Correct usage:
		@verbatim
		TiXmlDocument doc;
		doc.SetTabSize( 8 );
		doc.Load( "myfile.xml" );
		@endverbatim

		@sa Row, Column
	*/
	void SetTabSize( int _tabsize )		{ tabsize = _tabsize; }

	int TabSize() const	{ return tabsize; }

	/** If you have handled the error, it can be reset with this call. The error
		state is automatically cleared if you Parse a new XML block.
	*/
	void ClearError()						{	error = false; 
												errorId = 0; 
												errorDesc = ""; 
												errorLocation.row = errorLocation.col = 0; 
												//errorLocation.last = 0; 
											}

	/** Write the document to standard out using formatted printing ("pretty print"). */
	void Print() const						{ Print( stdout, 0 ); }

	/* Write the document to a string using formatted printing ("pretty print"). This
		will allocate a character array (new char[]) and return it as a pointer. The
		calling code pust call delete[] on the return char* to avoid a memory leak.
	*/
	//char* PrintToMemory() const; 

	/// Print this Document to a FILE stream.
	virtual void Print( FILE* cfile, int depth = 0 ) const;
	// [internal use]
	void SetError( int err, const char* errorLocation, TiXmlParsingData* prevData, TiXmlEncoding encoding );

	virtual const TiXmlDocument*    ToDocument()    const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlDocument*          ToDocument()          { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

	/** Walk the XML tree visiting this node and all of its children. 
	*/
	virtual bool Accept( TiXmlVisitor* content ) const;

protected :
	// [internal use]
	virtual TiXmlNode* Clone() const;
	#ifdef TIXML_USE_STL
	virtual void StreamIn( std::istream * in, TIXML_STRING * tag );
	#endif

private:
	void CopyTo( TiXmlDocument* target ) const;

	bool error;
	int  errorId;
	TIXML_STRING errorDesc;
	int tabsize;
	TiXmlCursor errorLocation;
	bool useMicrosoftBOM;		// the UTF-8 BOM were found when read. Note this, and try to write.
};


/**
	A TiXmlHandle is a class that wraps a node pointer with null checks; this is
	an incredibly useful thing. Note that TiXmlHandle is not part of the TinyXml
	DOM structure. It is a separate utility class.

	Take an example:
	@verbatim
	<Document>
		<Element attributeA = "valueA">
			<Child attributeB = "value1" />
			<Child attributeB = "value2" />
		</Element>
	<Document>
	@endverbatim

	Assuming you want the value of "attributeB" in the 2nd "Child" element, it's very 
	easy to write a *lot* of code that looks like:

	@verbatim
	TiXmlElement* root = document.FirstChildElement( "Document" );
	if ( root )
	{
		TiXmlElement* element = root->FirstChildElement( "Element" );
		if ( element )
		{
			TiXmlElement* child = element->FirstChildElement( "Child" );
			if ( child )
			{
				TiXmlElement* child2 = child->NextSiblingElement( "Child" );
				if ( child2 )
				{
					// Finally do something useful.
	@endverbatim

	And that doesn't even cover "else" cases. TiXmlHandle addresses the verbosity
	of such code. A TiXmlHandle checks for null	pointers so it is perfectly safe 
	and correct to use:

	@verbatim
	TiXmlHandle docHandle( &document );
	TiXmlElement* child2 = docHandle.FirstChild( "Document" ).FirstChild( "Element" ).Child( "Child", 1 ).ToElement();
	if ( child2 )
	{
		// do something useful
	@endverbatim

	Which is MUCH more concise and useful.

	It is also safe to copy handles - internally they are nothing more than node pointers.
	@verbatim
	TiXmlHandle handleCopy = handle;
	@endverbatim

	What they should not be used for is iteration:

	@verbatim
	int i=0; 
	while ( true )
	{
		TiXmlElement* child = docHandle.FirstChild( "Document" ).FirstChild( "Element" ).Child( "Child", i ).ToElement();
		if ( !child )
			break;
		// do something
		++i;
	}
	@endverbatim

	It seems reasonable, but it is in fact two embedded while loops. The Child method is 
	a linear walk to find the element, so this code would iterate much more than it needs 
	to. Instead, prefer:

	@verbatim
	TiXmlElement* child = docHandle.FirstChild( "Document" ).FirstChild( "Element" ).FirstChild( "Child" ).ToElement();

	for( child; child; child=child->NextSiblingElement() )
	{
		// do something
	}
	@endverbatim
*/
class TiXmlHandle
{
public:
	/// Create a handle from any node (at any depth of the tree.) This can be a null pointer.
	TiXmlHandle( TiXmlNode* _node )					{ this->node = _node; }
	/// Copy constructor
	TiXmlHandle( const TiXmlHandle& ref )			{ this->node = ref.node; }
	TiXmlHandle operator=( const TiXmlHandle& ref ) { this->node = ref.node; return *this; }

	/// Return a handle to the first child node.
	TiXmlHandle FirstChild() const;
	/// Return a handle to the first child node with the given name.
	TiXmlHandle FirstChild( const char * value ) const;
	/// Return a handle to the first child element.
	TiXmlHandle FirstChildElement() const;
	/// Return a handle to the first child element with the given name.
	TiXmlHandle FirstChildElement( const char * value ) const;

	/** Return a handle to the "index" child with the given name. 
		The first child is 0, the second 1, etc.
	*/
	TiXmlHandle Child( const char* value, int index ) const;
	/** Return a handle to the "index" child. 
		The first child is 0, the second 1, etc.
	*/
	TiXmlHandle Child( int index ) const;
	/** Return a handle to the "index" child element with the given name. 
		The first child element is 0, the second 1, etc. Note that only TiXmlElements
		are indexed: other types are not counted.
	*/
	TiXmlHandle ChildElement( const char* value, int index ) const;
	/** Return a handle to the "index" child element. 
		The first child element is 0, the second 1, etc. Note that only TiXmlElements
		are indexed: other types are not counted.
	*/
	TiXmlHandle ChildElement( int index ) const;

	#ifdef TIXML_USE_STL
	TiXmlHandle FirstChild( const std::string& _value ) const				{ return FirstChild( _value.c_str() ); }
	TiXmlHandle FirstChildElement( const std::string& _value ) const		{ return FirstChildElement( _value.c_str() ); }

	TiXmlHandle Child( const std::string& _value, int index ) const			{ return Child( _value.c_str(), index ); }
	TiXmlHandle ChildElement( const std::string& _value, int index ) const	{ return ChildElement( _value.c_str(), index ); }
	#endif

	/** Return the handle as a TiXmlNode. This may return null.
	*/
	TiXmlNode* ToNode() const			{ return node; } 
	/** Return the handle as a TiXmlElement. This may return null.
	*/
	TiXmlElement* ToElement() const		{ return ( ( node && node->ToElement() ) ? node->ToElement() : 0 ); }
	/**	Return the handle as a TiXmlText. This may return null.
	*/
	TiXmlText* ToText() const			{ return ( ( node && node->ToText() ) ? node->ToText() : 0 ); }
	/** Return the handle as a TiXmlUnknown. This may return null.
	*/
	TiXmlUnknown* ToUnknown() const		{ return ( ( node && node->ToUnknown() ) ? node->ToUnknown() : 0 ); }

	/** @deprecated use ToNode. 
		Return the handle as a TiXmlNode. This may return null.
	*/
	TiXmlNode* Node() const			{ return ToNode(); } 
	/** @deprecated use ToElement. 
		Return the handle as a TiXmlElement. This may return null.
	*/
	TiXmlElement* Element() const	{ return ToElement(); }
	/**	@deprecated use ToText()
		Return the handle as a TiXmlText. This may return null.
	*/
	TiXmlText* Text() const			{ return ToText(); }
	/** @deprecated use ToUnknown()
		Return the handle as a TiXmlUnknown. This may return null.
	*/
	TiXmlUnknown* Unknown() const	{ return ToUnknown(); }

private:
	TiXmlNode* node;
};


/** Print to memory functionality. The TiXmlPrinter is useful when you need to:

	-# Print to memory (especially in non-STL mode)
	-# Control formatting (line endings, etc.)

	When constructed, the TiXmlPrinter is in its default "pretty printing" mode.
	Before calling Accept() you can call methods to control the printing
	of the XML document. After TiXmlNode::Accept() is called, the printed document can
	be accessed via the CStr(), Str(), and Size() methods.

	TiXmlPrinter uses the Visitor API.
	@verbatim
	TiXmlPrinter printer;
	printer.SetIndent( "\t" );

	doc.Accept( &printer );
	fprintf( stdout, "%s", printer.CStr() );
	@endverbatim
*/
class TiXmlPrinter : public TiXmlVisitor
{
public:
	TiXmlPrinter() : depth( 0 ), simpleTextPrint( false ),
					 buffer(), indent( "    " ), lineBreak( "\n" ) {}

	virtual bool VisitEnter( const TiXmlDocument& doc );
	virtual bool VisitExit( const TiXmlDocument& doc );

	virtual bool VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute );
	virtual bool VisitExit( const TiXmlElement& element );

	virtual bool Visit( const TiXmlDeclaration& declaration );
	virtual bool Visit( const TiXmlText& text );
	virtual bool Visit( const TiXmlComment& comment );
	virtual bool Visit( const TiXmlUnknown& unknown );

	/** Set the indent characters for printing. By default 4 spaces
		but tab (\t) is also useful, or null/empty string for no indentation.
	*/
	void SetIndent( const char* _indent )			{ indent = _indent ? _indent : "" ; }
	/// Query the indention string.
	const char* Indent()							{ return indent.c_str(); }
	/** Set the line breaking string. By default set to newline (\n). 
		Some operating systems prefer other characters, or can be
		set to the null/empty string for no indenation.
	*/
	void SetLineBreak( const char* _lineBreak )		{ lineBreak = _lineBreak ? _lineBreak : ""; }
	/// Query the current line breaking string.
	const char* LineBreak()							{ return lineBreak.c_str(); }

	/** Switch over to "stream printing" which is the most dense formatting without 
		linebreaks. Common when the XML is needed for network transmission.
	*/
	void SetStreamPrinting()						{ indent = "";
													  lineBreak = "";
													}	
	/// Return the result.
	const char* CStr()								{ return buffer.c_str(); }
	/// Return the length of the result string.
	size_t Size()									{ return buffer.size(); }

	#ifdef TIXML_USE_STL
	/// Return the result.
	const std::string& Str()						{ return buffer; }
	#endif

private:
	void DoIndent()	{
		for( int i=0; i<depth; ++i )
			buffer += indent;
	}
	void DoLineBreak() {
		buffer += lineBreak;
	}

	int depth;
	bool simpleTextPrint;
	TIXML_STRING buffer;
	TIXML_STRING indent;
	TIXML_STRING lineBreak;
};


#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif

