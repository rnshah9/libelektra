/***************************************************************************
                 stream.c  -  Streaming Methods
                             -------------------
    begin                : Sat Nov 23 2007
    copyright            : (C) 2007 by Markus Raab
    email                : elektra@markus-raab.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/

#include "xmltool.h"

#ifdef HAVE_KDBCONFIG_H
#include "kdbconfig.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <kdbtools.h>
#include <kdbinternal.h>


/**
 * @brief Methods to output, generate and toXML Keys and Keysets.
 *
 * Here are some functions that are in a separate library because they
 * depend on non-basic libraries as libxml. Link against kdbtools library if your
 * program won't be installed in /bin, or is not essential in early boot
 * stages.
 *
 * It is also possible to have a dynamic linkage, see the
 * sourcecode from kdb-tool or testing framework how to achieve
 * that.
 *
 * Output prints keys line per line, meaned to be read by humans.
 * - keyOutput()
 * - ksOutput()
 *
 * Generate prints keys and keysets as C-Structs, meaned to be
 * read by a C-Compiler.
 * - keyGenerate()
 * - ksGenerate()
 *
 * toXML prints keys and keysets as XML, meaned to be used
 * as exchange format.
 * - keyToStream()
 * - ksToStream()
 *
 * To use them:
 * @code
#include <kdbtools.h>
 * @endcode
 *
 *
 */






/*********************************************
 *    Textual XML methods                    *
 *********************************************/


/**
 * Prints an XML representation of the key.
 *
 * String generated is of the form:
 * @verbatim
	<key name="system/sw/xorg/Monitor/Monitor0/Name"
		type="string" uid="root" gid="root" mode="0660">

		<value>Samsung TFT panel</value>
		<comment>My monitor</comment>
	</key>@endverbatim



 * @verbatim
	<key parent="system/sw/xorg/Monitor/Monitor0" basename="Name"
		type="string" uid="root" gid="root" mode="0660">

		<value>Samsung TFT panel</value>
		<comment>My monitor</comment>
	</key>@endverbatim
 *
 * @param key the key object to work with
 * @param stream where to write output: a file or stdout
 * @param options Some #option_t ORed:
 * - @p option_t::KDB_O_NUMBERS \n
 *   Do not convert UID and GID into user and group names
 * - @p option_t::KDB_O_CONDENSED \n
 *   Less human readable, more condensed output
 * - @p option_t::KDB_O_FULLNAME \n
 *   The @p user keys are exported with their full names (including
 *   user domains)
 *
 * @see ksToStream()
 * @return number of bytes written to output
 */
ssize_t keyToStream(const Key *key, FILE* stream, option_t options)
{
	return keyToStreamBasename(key,stream,0,0,options);
}





/**
 * Same as keyToStream() but tries to strip @p parentSize bytes from
 * @p key name if it matches @p parent .
 *
 * Taking the example from keyToStream(), if @p parent is
 * @c "system/sw/xorg", the generated string is of the form:
 * @verbatim
	<key basename="Monitor/Monitor0/Name"
		type="string" uid="root" gid="root" mode="0660">

		<value>Samsung TFT panel</value>
		<comment>My monitor</comment>
	</key>@endverbatim
 *
 * It usefull to produce more human readable XML output of a key when
 * it is being represented in a context that defines the parent key name.
 * For example:
 *
 * @verbatim
	<keyset parent="user/sw">
		<key basename="kdbedit"..../>
		<key basename="phototools"..../>
		<key basename="myapp"..../>
	</keyset>@endverbatim
 *
 * In the bove example, each @p @<key@> entry was generated by a call to 
 * keyToStreamBasename() having @c "user/sw" as @p parent .
 * 
 * This method is used when ksToStream() is called with
 * KDBOption::KDB_O_HIER option.
 *
 * @param key the key object to work with
 * @param stream the FILE where to send the stream
 * @param parentSize the maximum size of @p parent that will be used.
 *        If 0, the entire @p parent will be used.
 * @param parent the string (or part of it, defined by @p parentSize ) that
 *        will be used to strip from the key name.
 * @param options Some #option_t ORed:
 * - @p option_t::KDB_O_NUMBERS \n
 *   Do not convert UID and GID into user and group names
 * - @p option_t::KDB_O_CONDENSED \n
 *   Less human readable, more condensed output
 * - @p option_t::KDB_O_FULLNAME \n
 *   The @p user keys are exported with their full names (including
 *   user domains)
 *
 * @return number of bytes written to output
 */
ssize_t keyToStreamBasename(const Key *key, FILE *stream, const char *parent,
		const size_t parentSize, option_t options) {
	ssize_t written=0;
	char buffer[KDB_MAX_PATH_LENGTH];

	uid_t uid = getuid();
	gid_t gid = getgid();

	/* Write key name */
	if (parent) {
		/* some logic to see if we should print only the relative basename */
		int found;
		size_t skip=parentSize ? parentSize : elektraStrLen(parent)-1;

		found=memcmp(parent,key->key,skip);
		if (found == 0) {
			while (*(key->key+skip) == KDB_PATH_SEPARATOR) ++skip;

			if (*(key->key+skip) != 0) /* we don't want a null basename */
				written+=fprintf(stream,"<key basename=\"%s\"",
					key->key+skip);
		}
	}

	if (written == 0) { /* no "<key basename=..." was written so far */
		if (options & KDB_O_FULLNAME) {
			keyGetFullName(key,buffer,sizeof(buffer));
			written+=fprintf(stream,"<key name=\"%s\"", buffer);
		} else written+=fprintf(stream,"<key name=\"%s\"", key->key);
	}


	if (options & KDB_O_CONDENSED) written+=fprintf(stream," ");
	else written+=fprintf(stream,"\n       ");



	/* Key type
	TODO: xml schema does not output type
	if (options & KDB_O_NUMBERS) {
		written+=fprintf(stream," type=\"%d\"", key->type);
	} else {
		buffer[0]=0;

		if (key->type & KEY_TYPE_DIR) written+=fprintf(stream, " isdir=\"yes\"");
		if (key->type & KEY_TYPE_REMOVE) written+=fprintf(stream, " isremove=\"yes\"");
		if (key->type & KEY_TYPE_BINARY) written+=fprintf(stream, " isbinary=\"yes\"");
	}
	*/

	if (keyGetUID (key) != uid) written+=fprintf(stream," uid=\"%d\"", (int)keyGetUID (key));
	if (keyGetGID (key) != gid) written+=fprintf(stream," gid=\"%d\"", (int)keyGetGID (key));

#if defined(S_IRWXU) && defined(S_IRWXG) && defined(S_IRWXO)
	written+=fprintf(stream," mode=\"0%o\"",
		key->mode & (S_IRWXU|S_IRWXG|S_IRWXO));
#else
	written+=fprintf(stream," mode=\"0%o\"",
		keyGetMode(key));
#endif


	if (!key->data.v && !keyComment(key)) { /* no data AND no comment */
		written+=fprintf(stream,"/>");
		if (!(options & KDB_O_CONDENSED))
			written+=fprintf(stream,"\n\n");
		
		return written; /* end of <key/> */
	} else {
		if (key->data.v) {
			if ((key->dataSize <= 16) && keyIsString (key) && /*TODO: is this for string?*/
					!strchr(key->data.c,'\n')) {

				/* we'll use a "value" attribute instead of a <value> node,
				   for readability, so the cut size will be 16, which is
				   the maximum size of an IPv4 address */

				if (options & KDB_O_CONDENSED) written+=fprintf(stream," ");
				else written+=fprintf(stream,"\n\t");
				
				written+=fprintf(stream,"value=\"%s\"",key->data.c);
				
				if (keyComment(key)) written+=fprintf(stream,">\n");
				else {
					written+=fprintf(stream,"/>");
					if (!(options & KDB_O_CONDENSED))
						written+=fprintf(stream,"\n");
				
					return written;
				}
			} else { /* value is bigger than 16 bytes: deserves own <value> */
				written+=fprintf(stream,">");
				if (!(options & KDB_O_CONDENSED)) written+=fprintf(stream,"\n\n     ");
				
				written+=fprintf(stream,"<value>");
				if (keyIsString(key)) { /*TODO: is this for string?*/
					written+=fprintf(stream,"<![CDATA[");
					fflush(stream);
					/* must chop ending \\0 */
					written+=fwrite(key->data.v,sizeof(char),key->dataSize-1,stream);
					written+=fprintf(stream,"]]>");
				} else {
					/* TODO Binary values 
					char *encoded=malloc(3*key->dataSize);
					size_t encodedSize;

					written+=fprintf(stream,"\n");
					encodedSize=kdbbEncode(key->data.c,key->dataSize,encoded);
					fflush(stream);
					written+=fwrite(encoded,sizeof(char),encodedSize,stream);
					free(encoded);
					written+=fprintf(stream,"\n");
					*/
				}
				/* fflush(stream); */
				written+=fprintf(stream,"</value>");
			}
		} else { /* we have no data */
			if (keyComment(key)) {
				written+=fprintf(stream,">");
				if (!(options & KDB_O_CONDENSED))
					written+=fprintf(stream,"\n");
			} else {
				written+=fprintf(stream,"/>");
				if (!(options & KDB_O_CONDENSED))
					written+=fprintf(stream,"\n\n");
			
				return written;
			}
		}
	}

	if (!(options & KDB_O_CONDENSED)) {
		written+=fprintf(stream,"\n");
		if (keyComment(key)) written+=fprintf(stream,"     ");
	}

	if (keyComment(key)) {
		written+=fprintf(stream,"<comment><![CDATA[%s]]></comment>", keyComment(key));
		if (!(options & KDB_O_CONDENSED))
			written+=fprintf(stream,"\n");
	}

	written+=fprintf(stream,"</key>");

	if (!(options & KDB_O_CONDENSED))
		written+=fprintf(stream,"\n\n");

	return written;
}

/**
 * Writes to @p stream an XML version of the @p ks object.
 *
 * String generated is of the form:
 * @verbatim
<keyset>
<key name=...>...</key>
<key name=...>...</key>
<key name=...>...</key>

</keyset>
 * @endverbatim
 *
 * or if KDB_O_HIER is used, the form will be:
 * @verbatim
<keyset parent="user/smallest/parent/name">

<key basename=...>...</key>
<key name=...>...</key> <!-- a key thats not under this keyset's parent -->
<key basename=...>...</key>

</keyset>
 * @endverbatim
 *
 * KDB_O_HEADER will additionally generate a header like:
 * @verbatim
<?xml version="1.0" encoding="UTF-8"?>
<!-- Generated by Elektra API. Total of n keys. -->
<keyset xmlns="http://www.libelektra.org"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xsi:schemaLocation="http://www.libelektra.org elektra.xsd">
 * @endverbatim
 *
 * @param ks the KeySet to serialise
 * @param stream where to write output: a file or stdout
 * @param options accepted #option_t ORed:
 * - @p option_t::KDB_O_NUMBERS \n
 *   Do not convert UID and GID into user and group names.
 * - @p option_t::KDB_O_FULLNAME \n
 *   The @c user keys are exported with their full names (including
 *   user domains)
 * - @p option_t::KDB_O_CONDENSED \n
 *   Less human readable, more condensed output.
 * - @p option_t::KDB_O_XMLHEADERS \n
 *   Exclude the correct XML headers in the output. If not used, the
 *   &lt;?xml?&gt; and schema info inside the &lt;keyset&gt; object will not be generated.
 * - @p option_t::KDB_O_HIER \n
 *   Will generate a &lt;keyset&gt; node containing a @c parent attribute, and
 *   &lt;key&gt; nodes with a @c basename relative to that @c parent. The @c parent
 *   is calculated by taking the smallest key name in the keyset, so it is a
 *   good idea to have only related keys on the keyset. Otherwise, a valid
 *   consistent XML document still will be generated with regular absolute
 *   @c name attribute for the &lt;key&gt; nodes, due to a
 *   clever keyToStreamBasename() implementation.
 *
 * @see keyToStream()
 * @see commandList() for usage example
 * @return number of bytes written to output, or -1 if some error occurs
 * @param ks The keyset to output
 * @param stream the file pointer where to send the stream
 * @param options see above text
 */
ssize_t ksToStream(const KeySet *ks, FILE* stream, option_t options)
{
	size_t written=0;
	Key *key=0;
	char *codeset = "UTF-8";
	KeySet *cks = ksDup (ks);

	ksRewind (cks);

	if (options & KDB_O_HEADER) {
		written+=fprintf(stream,"<?xml version=\"1.0\" encoding=\"%s\"?>",
			codeset);
		if (~options & KDB_O_CONDENSED)
			written+=fprintf(stream,
					"\n<!-- Generated by Elektra API. Total of %d keys. -->\n",(int)cks->size);
		if (~options & KDB_O_CONDENSED)
			written+=fprintf(stream,"<keyset xmlns=\"http://www.libelektra.org\"\n"
					"\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
					"\txsi:schemaLocation=\"http://www.libelektra.org elektra.xsd\"\n");
		else
			written+=fprintf(stream,"<keyset xmlns=\"http://www.libelektra.org\""
					" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
					" xsi:schemaLocation=\"http://www.libelektra.org elektra.xsd\"");
	} else written+=fprintf(stream,"<keyset");

	if (options & KDB_O_HIER) {
		char commonParent[KDB_MAX_PATH_LENGTH];

		ksGetCommonParentName(cks,commonParent,sizeof(commonParent));
	
		if (commonParent[0]) {
			written+=fprintf(stream,"        parent=\"%s\">\n",
				commonParent);
			ksRewind (cks);
			while ((key=ksNext (cks)) != 0)
				written+=keyToStreamBasename(key,stream,commonParent,0,options);
		} else {
			written+=fprintf(stream,">\n");
			ksRewind (cks);
			while ((key=ksNext (cks)) != 0)
				written+=keyToStream(key,stream,options);
		}
	} else { /* No KDB_O_HIER*/
		written+=fprintf(stream,">\n");
		ksRewind (cks);
		while ((key=ksNext (cks)) != 0)
			written+=keyToStream(key,stream,options);
	}
	
	written+=fprintf(stream,"</keyset>\n");
	ksDel (cks);
	return written;
}

/**
 * Output every information of a single key depending on options.
 *
 * The format is not very strict and only intend to be read
 * by human eyes for debugging purposes. Don't rely on the
 * format in your applications.
 *
 * @param k the key object to work with
 * @param stream the file pointer where to send the stream
 * @param options see text above
 * @see ksOutput()
 * @return 1 on success
 * @return -1 on allocation errors
 * @ingroup stream
 */
int keyOutput (const Key * k, FILE *stream, option_t options)
{
	time_t t;
	size_t s;
	char * tmc;
	char * str;

	size_t c;
	char * com;

	size_t n;
	char * nam;

	n = keyGetNameSize (k);
	if (n>1)
	{
		nam = (char*) malloc (n);
		if (nam == NULL) return -1;
		keyGetName (k, nam, n);

		fprintf(stream,"Name[%d]: %s : ", (int)n, nam);

		free (nam);
	}

	s = keyGetValueSize (k);
	if (options & KEY_VALUE && s>1)
	{
		str = (char*) malloc (s);
		if (str == NULL) return -1;
		if (keyIsBinary(k))
		{
			/*
			char * bin;
			bin = (char*) malloc (s*3+1);
			keyGetBinary(k, str, s);
			kdbbEncode (str, s, bin);
			free (bin);
			*/
			keyGetBinary (k, str, s);
			fprintf(stream,"Binary[%d]: %s : ", (int)s, str);
		} else {
			keyGetString (k, str, s);
			fprintf(stream,"String[%d]: %s : ", (int)s, str);
		}

		free (str);
	}

	c = keyGetCommentSize (k);
	if (options & KEY_COMMENT && c>1)
	{
		com = (char*) malloc (c);
		if (com == NULL) return -1;
		keyGetComment (k, com, c);

		fprintf(stream,"Comment[%d]: %s : ", (int)c, com);

		free (com);
	}


	if (options & KDB_O_SHOWMETA) fprintf(stream," : ");
	if (options & KEY_UID) fprintf(stream,"UID: %d : ", (int)keyGetUID (k));
	if (options & KEY_GID) fprintf(stream,"GID: %d : ", (int)keyGetGID (k));
	if (options & KEY_MODE) fprintf(stream,"Mode: %o : ", (int)keyGetMode (k));

	if (options & KEY_ATIME)
	{
		t=keyGetATime(k);
		tmc = ctime (& t);
		tmc[24] = '\0';
		fprintf(stream,"ATime: %s : ", tmc);
	}

	if (options & KEY_MTIME)
	{
		t=keyGetMTime(k);
		tmc = ctime (& t);
		tmc[24] = '\0';
		fprintf(stream,"MTime: %s : ", tmc);
	}

	if (options & KEY_CTIME)
	{
		t=keyGetCTime(k);
		tmc = ctime (& t);
		tmc[24] = '\0';
		fprintf(stream,"CTime: %s : ", tmc);
	}

	if (options & KDB_O_SHOWFLAGS)
	{
		if (!(options & KDB_O_SHOWMETA)) fprintf(stream, " ");
		fprintf (stream,"Flags: ");
		if (keyIsBinary(k)) fprintf(stream,"b");
		if (keyIsString(k)) fprintf(stream,"s");
		if (keyIsDir(k)) fprintf(stream,"d");
		if (keyIsInactive(k)) fprintf(stream,"i");
		if (keyNeedSync(k)) fprintf(stream,"s");
	}

	fprintf(stream,"\n");
	return 1;
}


/**
 * Output all information of a keyset.
 *
 * The format is not very strict and only intend to be read
 * by human eyes for debugging purposes. Don't rely on the
 * format in your applications.
 *
 * Keys are printed line per line with keyOutput().
 *
 * The same options as keyOutput() are taken and passed
 * to them.
 *
 * Additional KDB_O_HEADER will print the number of keys
 * as first line.
 *
 * @param ks the keyset to work with
 * @param stream the file pointer where to send the stream
 * @param options
 * @see keyOutput()
 * @return 1 on success
 * @return -1 on allocation errors
 * @ingroup stream
 */
int ksOutput(const KeySet *ks, FILE *stream, option_t options)
{
	Key	*key;
	KeySet *cks = ksDup (ks);
	size_t size = 0;

	ksRewind (cks);

	if (KDB_O_HEADER & options) {
		fprintf(stream,"Output keyset of size %d\n", (int)ksGetSize(cks)); 
	}
	while ( (key = ksNext(cks)) != NULL)
	{
		if (options & KDB_O_SHOWINDICES) fprintf(stream, "[%d] ", (int)size);
		keyOutput (key,stream,options);
		size ++;
	}

	ksDel (cks);
	return 1;
}

/**
 * Generate a C-Style key and stream it.
 *
 * This keyset can be used to include as c-code for
 * applikations using elektra.
 *
 * @param key the key object to work with
 * @param stream the file pointer where to send the stream
 * @param options KDB_O_SHOWINDICES, KDB_O_IGNORE_COMMENT, KDB_O_SHOWINFO
 * @return 1 on success
 * @ingroup stream
 */
int keyGenerate(const Key * key, FILE *stream, option_t options)
{
	size_t s;
	char * str;

	size_t c;
	char * com;

	size_t n;
	char * nam;

	n = keyGetNameSize (key);
	if (n>1)
	{
		nam = (char*) malloc (n);
		if (nam == NULL) return -1;
		keyGetName (key, nam, n);
		fprintf(stream,"\tkeyNew (\"%s\"", nam);
		free (nam);
	}

	if (keyIsDir(key)) fprintf(stream,", KEY_DIR");

	s = keyGetValueSize (key);
	if (s>1)
	{
		str = (char*) malloc (s);
		if (str == NULL) return -1;
		if (keyIsBinary(key)) keyGetBinary(key, str, s);
		else keyGetString (key, str, s);
		fprintf(stream,", KEY_VALUE, \"%s\"", str);
		free (str);
	}

	c = keyGetCommentSize (key);
	if (c>1)
	{
		com = (char*) malloc (c);
		if (com == NULL) return -1;
		keyGetComment (key, com, c);
		fprintf(stream,", KEY_COMMENT, \"%s\"", com);
		free (com);
	}

	if (! (keyGetMode(key) == 0664 || (keyGetMode(key) == 0775 && keyIsDir(key))))
	{
		fprintf(stream,", KEY_MODE, 0%3o", keyGetMode(key));
	}

	fprintf(stream,", KEY_END)");

	if (options == 0) return 1; /* dummy to make icc happy */
	return 1;
}


/**
 * Generate a C-Style keyset and stream it.
 *
 * This keyset can be used to include as c-code for
 * applikations using elektra.
 *
 * The options takes the same options as kdbGet()
 * and kdbSet().
 *
 * @param ks the keyset to work with
 * @param stream the file pointer where to send the stream
 * @param options which keys not to output
 * @return 1 on success
 * @ingroup stream
 */
int ksGenerate (const KeySet *ks, FILE *stream, option_t options)
{
	Key	*key;
	size_t  s = 0;
	KeySet *cks = ksDup (ks);

	ksRewind (cks);

	fprintf(stream,"ksNew( %d ,\n", (int)ksGetSize(cks));
	while ((key=ksNext(cks)) != 0)
	{
		if (options & KDB_O_NODIR) if (key && keyIsDir (key)) continue;
		if (options & KDB_O_DIRONLY) if (key && !keyIsDir (key)) continue;
		if (options & KDB_O_INACTIVE) if (key && keyIsInactive (key)) continue;

		s++;

		keyGenerate(key, stream, options);
		fprintf(stream,",\n");
	}
	fprintf(stream,"\tKS_END);\n"); 

	ksDel (cks);
	return 1;
}

