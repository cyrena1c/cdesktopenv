/*
 * CDE - Common Desktop Environment
 *
 * Copyright (c) 1993-2012, The Open Group. All rights reserved.
 *
 * These libraries and programs are free software; you can
 * redistribute them and/or modify them under the terms of the GNU
 * Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * These libraries and programs are distributed in the hope that
 * they will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with these libraries and programs; if not, write
 * to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301 USA
 */
/*
 *  Copyright 1993 Open Software Foundation, Inc., Cambridge, Massachusetts.
 *  All rights reserved.
 */
/*
 * Copyright (c) 1994  
 * Open Software Foundation, Inc. 
 *  
 * Permission is hereby granted to use, copy, modify and freely distribute 
 * the software in this file and its documentation for any purpose without 
 * fee, provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation.  Further, provided that the name of Open 
 * Software Foundation, Inc. ("OSF") not be used in advertising or 
 * publicity pertaining to distribution of the software without prior 
 * written permission from OSF.  OSF makes no representations about the 
 * suitability of this software for any purpose.  It is provided "as is" 
 * without express or implied warranty. 
 */
/* ________________________________________________________________________
 *
 *  General utility functions for 'instant' program.  These are used
 *  throughout the rest of the program.
 *
 *  Entry points for this module:
 *	Split(s, &n, flags)		split string into n tokens
 *	NewMap(slot_incr)		create a new mapping structure
 *	FindMapping(map, name)		find mapping by name; return mapping
 *	FindMappingVal(map, name)	find mapping by name; return value
 *	SetMapping(map, s)		set mapping based on string
 *	OpenFile(filename)		open file, looking in inst path
 *	FindElementPath(elem, s)	find path to element
 *	PrintLocation(ele, fp)		print location of element in tree
 *	NearestOlderElem(elem, name)	find prev elem up tree with name
 *	OutputString(s, fp, track_pos)	output string
 *	AddElemName(name)		add elem to list of known elements
 *	AddAttName(name)		add att name to list of known atts
 *	FindAttByName(elem, name)	find an elem's att by name
 *	FindContext(elem, lev, context)	find context of elem
 *	QRelation(elem, name, rel_flag)	find relation elem has to named elem
 *	DescendTree(elem, enter_f, leave_f, data_f, dp)	descend doc tree,
 *					calling functions for each elem/node
 * ________________________________________________________________________
 */

#ifndef lint
static char *RCSid =
  "$TOG: util.c /main/13 1997/10/09 16:09:50 bill $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#if !defined(CSRG_BASED)
#include <values.h>
#endif

#include "general.h"

/* forward references */
static char	*LookupSDATA(char *);
static int       CheckOutputBuffer(int length);

static OutputBuffer_t outputBuffer; /* init'd to all 0 by compiler */

/* ______________________________________________________________________ */
/*  "Split" a string into tokens.  Given a string that has space-separated
 *  (space/tab) tokens, return a pointer to an array of pointers to the
 *  tokens.  Like what the shell does with *argv[].  The array can be is
 *  static or allocated.  Space can be allocated for string, or allocated.
 *  Arguments:
 *	Pointer to string to pick apart.
 *	Pointer to max number of tokens to find; actual number found is
 *	  returned. If 0 or null pointer, use a 'sane' maximum number (hard-
 *	  code). If more tokens than the number specified, make last token be
 *	  a single string composed of the rest of the tokens (includes spaces).
 *	Flag. Bit 0 says whether to make a copy of input string (since we'll
 *	  clobber parts of it).  To free the string, use the pointer to
 *	  the first token returned by the function (or *ret_value).
 *	  Bit 1 says whether to allocate the vector itself.  If not, use
 *	  (and return) a static vector.
 *  Return:
 *	Pointer to the provided string (for convenience of caller).
 */

char **
Split(
    char	*s,		/* input string */
    int		*ntok,		/* # of tokens desired (input)/found (return) */
    int		flag		/* dup string? allocate a vector? */
)
{
    int		maxnt, i=0;
    int		n_alloc;
    char	**tokens;
    static char	*local_tokens[100];

    /* Figure max number of tokens (maxnt) to find.  0 means find them all. */
    if (ntok == NULL)
	maxnt = 100;
    else {
	if (*ntok <= 0 || *ntok > 100) maxnt = 100;	/* arbitrary size */
	else maxnt = *ntok;
	*ntok = 0;
    }

    if (!s) return 0;			/* no string */

    /* Point to 1st token (there may be initial space) */
    while (*s && IsWhite(*s)) s++;	/* skip initial space, if any */
    if (*s == EOS) return 0;		/* none found? */

    /* See if caller wants us to copy the input string. */
    if (flag & S_STRDUP) s = strdup(s);

    /* See if caller wants us to allocate the returned vector. */
    if (flag & S_ALVEC) {
	n_alloc = 20;
	Malloc(n_alloc, tokens, char *);
	/* if caller did not specify max tokens to find, set to more than
	 * there will possibly ever be */
	if (!ntok || !(*ntok)) maxnt = 10000;
    }
    else tokens = local_tokens;

    i = 0;			/* index into vector */
    tokens[0] = s;		/* s already points to 1st token */
    while (i<maxnt) {
	tokens[i] = s;		/* point vector member at start of token */
	i++;
	/* If we allocated vector, see if we need more space. */
	if ((flag & S_ALVEC) && i >= n_alloc) {
	    n_alloc += 20;
	    Realloc(n_alloc, tokens, char *);
	}
	if (i >= maxnt) break;			/* is this the last one? */
	while (*s && !IsWhite(*s)) s++;		/* skip past end of token */
	if (*s == EOS) break;			/* at end of input string? */
	if (*s) *s++ = EOS;			/* terminate token string */
	while (*s && IsWhite(*s)) s++;		/* skip space - to next token */
    }
    if (ntok) *ntok = i;		/* return number of tokens found */
    tokens[i] = 0;			/* null-terminate vector */
    return tokens;
}

/* ______________________________________________________________________ */
/*  Mapping routines.  These are used for name-value pairs, like attributes,
 *  variables, and counters.  A "Map" is an opaque data structure used
 *  internally by these routines.  The caller gets one when creating a new
 *  map, then hands it to other routines that need it.  A "Mapping" is a
 *  name/value pair.  The user has access to this.
 *  Here's some sample usage:
 *
 *	Map *V;
 *	V = NewMap(20);
 *	SetMappingNV(V, "home", "/users/bowe");
 *	printf("Home: %s\n", FindMappingVal(V, "home");
 */

/*  Allocate new map structure.  Only done once for each map/variable list.
 *  Arg:
 *	Number of initial slots to allocate space for.  This is also the
 *	"chunk size" - how much to allocate when we use up the given space.
 *  Return:
 *	Pointer to the (opaque) map structure. (User passes this to other
 *	mapping routines.)
 */
Map_t *
NewMap(
    int		slot_increment
)
{
    Map_t	*M;
    Calloc(1, M, Map_t);
    if (!slot_increment) slot_increment = 1;
    M->slot_incr = slot_increment;
    return M;
}

/*  Given pointer to a Map and a name, find the mapping.
 *  Arguments:
 *	Pointer to map structure (as returned by NewMap().
 *	Variable name.
 *  Return:
 *	Pointer to the matching mapping structure, or null if not found.
 */
Mapping_t *
FindMapping(
    Map_t	*M,
    char	*name
)
{
    int		i;
    Mapping_t	*m;

    if (!M || M->n_used == 0) return NULL;
    for (m=M->maps,i=0; i<M->n_used; i++)
	if (m[i].name[0] == name[0] && !strcmp(m[i].name, name)) return &m[i];
    return NULL;

}

/*  Given pointer to a Map and a name, return string value of the mapping.
 *  Arguments:
 *	Pointer to map structure (as returned by NewMap().
 *	Variable name.
 *  Return:
 *	Pointer to the value (string), or null if not found.
 */
char *
FindMappingVal(
    Map_t	*M,
    char	*name
)
{
    Mapping_t	*m;
    if (!M || M->n_used == 0) return NULL;
    if ((m = FindMapping(M, name))) return m->sval;
    return NULL;

}

/*  Set a mapping/variable in Map M.  Input string is a name-value pair where
 *  there is some amount of space after the name.  The correct mapping is done.
 *  Arguments:
 *	Pointer to map structure (as returned by NewMap().
 *	Pointer to variable name (string).
 *	Pointer to variable value (string).
 */
void
SetMappingNV(
    Map_t	*M,
    char	*name,
    char	*value
)
{
    FILE	*pp;
    char	buf[LINESIZE], *cp, *s;
    int		i;
    Mapping_t	*m;

    /* First, look to see if it's a "well-known" variable. */
    if (!strcmp(name, "verbose"))  { verbose   = atoi(value); return; }
    if (!strcmp(name, "warnings")) { warnings  = atoi(value); return; }
    if (!strcmp(name, "foldcase")) { fold_case = atoi(value); return; }

    m = FindMapping(M, name);		/* find existing mapping (if set) */

    /* Need more slots for mapping structures?  Allocate in clumps. */
    if (M->n_used == 0) {
	M->n_alloc = M->slot_incr;
	Malloc(M->n_alloc, M->maps, Mapping_t);
    }
    else if (M->n_used >= M->n_alloc) {
	M->n_alloc += M->slot_incr;
	Realloc(M->n_alloc, M->maps, Mapping_t);
    }

    /* OK, we have a string mapping */
    if (m) {				/* exists - just replace value */
	free(m->sval);
	m->sval = strdup(value);
	if (value) m->sval = strdup(value);
	else m->sval = NULL;
    }
    else {
	if (name) {		/* just in case */
	    m = &M->maps[M->n_used];
	    M->n_used++;
	    m->name = strdup(name);
	    if (value) m->sval = strdup(value);
	    else m->sval = NULL;
	}
    }

    if (value)
    {
	/* See if the value is a command to run.  If so, run the command
	 * and replace the value with the output.
	 */
	s = value;
	if (*s == '!') {
	    s++;				/* point to command */
	    if ((pp = popen(s, "r"))) {		/* run cmd, read its output */
		i = 0;
		cp = buf;
		while (fgets(cp, LINESIZE-i, pp)) {
		    i += strlen(cp);
		    cp = &buf[i];
		    if (i >= LINESIZE) {
			fprintf(stderr,
			    "Prog execution of variable '%s' too long.\n",
			    m->name);
			break;
		    }
		}
		free(m->sval);
		stripNL(buf);
		m->sval = strdup(buf);
		pclose(pp);
	    }
	    else {
		sprintf(buf, "Could not start program '%s'", s);
		perror(buf);
	    }
	}
    }
}

/*  Separate name and value from input string, then pass to SetMappingNV.
 *  Arguments:
 *	Pointer to map structure (as returned by NewMap().
 *	Pointer to variable name and value (string), in form "name value".
 */
void
SetMapping(
    Map_t	*M,
    char	*s
)
{
    char	buf[LINESIZE];
    char	*name, *val;

    if (!M) {
	fprintf(stderr, "SetMapping: Map not initialized.\n");
	return;
    }
    snprintf(buf, sizeof(buf), "%s", s);
    name = val = buf;
    while (*val && !IsWhite(*val)) val++;	/* point past end of name */
    if (*val) {
	*val++ = EOS;				/* terminate name */
	while (*val && IsWhite(*val)) val++;	/* point to value */
    }
    if (name) SetMappingNV(M, name, val);
}

/* ______________________________________________________________________ */
/*  Opens a file for reading.  If not found in current directory, try
 *  lib directories (from TPT_LIB env variable, or -l option).
 *  Arguments:
 *	Filename (string).
 *  Return:
 *	FILE pointer to open file, or null if it not found or can't open.
 */

FILE *
OpenFile(
    char	*filename
)
{
    FILE	*fp;
    char	buf[LINESIZE];
    int		i;
    static char	**libdirs;
    static int	nlibdirs = -1;

    if ((fp=fopen(filename, "r"))) return fp;

    if (*filename == '/') return NULL;		/* full path specified? */

    if (nlibdirs < 0) {
	char *cp, *s;
	if (tpt_lib) {
	    s = strdup(tpt_lib);
	    for (cp=s; *cp; cp++) if (*cp == ':') *cp = ' ';
	    nlibdirs = 0;
	    libdirs = Split(s, &nlibdirs, S_ALVEC);
	}
	else nlibdirs = 0;
    }
    for (i=0; i<nlibdirs; i++) {
	sprintf(buf, "%s/%s", libdirs[i], filename);
	if ((fp=fopen(buf, "r"))) return fp;
    }
    return NULL;
}

/* ______________________________________________________________________ */
/*  This will find the path to an tag.  The format is the:
 *	tag1(n1):tag2(n2):tag3
 *  where the tags are going down the tree and the numbers indicate which
 *  child (the first is numbered 1) the next tag is.
 *  Returns pointer to the string just written to (so you can use this
 *  function as a printf arg).
 *  Arguments:
 *	Pointer to element under consideration.
 *	String to write path into (provided by caller).
 *  Return:
 *	Pointer to the provided string (for convenience of caller).
 */
char *
FindElementPath(
    Element_t	*e,
    char	*s
)
{
    Element_t	*ep;
    int		i, e_path[MAX_DEPTH];
    char	*cp;

    /* Move up the tree, noting "birth order" of each element encountered */
    for (ep=e; ep->parent; ep=ep->parent)
	e_path[ep->depth-1] = ep->my_eorder;
    /* Move down the tree, printing the element names to the string. */
    for (cp=s,i=0,ep=DocTree; i<e->depth; ep=ep->econt[e_path[i]],i++) {
	sprintf(cp, "%s(%d) ", ep->gi, e_path[i]);
	cp += strlen(cp);
    }
    sprintf(cp, "%s", e->gi);
    return s;
}

/* ______________________________________________________________________ */
/*  Print some location info about a tag.  Helps user locate error.
 *  Messages are indented 2 spaces (convention for multi-line messages).
 *  Arguments:
 *	Pointer to element under consideration.
 *	FILE pointer of where to print.
 */

void
PrintLocation(
    Element_t	*e,
    FILE	*fp
)
{
    char	*s, buf[LINESIZE];

    if (!e || !fp) return;
    fprintf(fp, "  Path: %s\n", FindElementPath(e, buf));
    if ((s=NearestOlderElem(e, "TITLE")))
	fprintf(fp, "  Position hint: TITLE='%s'\n", s);
    if (e->lineno) {
	if (e->infile)
	    fprintf(fp, "  At or near instance file: %s, line: %d\n",
			e->infile, e->lineno);
	else
	    fprintf(fp, "  At or near instance line: %d\n", e->lineno);
    }
    if (e->id)
	fprintf(fp, "  ID: %s\n", e->id);
}

/* ______________________________________________________________________ */
/*  Finds the data part of the nearest "older" tag (up the tree, and
 *  preceding) whose tag name matches the argument, or "TITLE", if null.
 *  Returns a pointer to the first chunk of character data.
 *  Arguments:
 *	Pointer to element under consideration.
 *	Name (GI) of element we'll return data from.
 *  Return:
 *	Pointer to that element's data content.
 */
char *
NearestOlderElem(
    Element_t	*e,
    char	*name
)
{
    int		i;
    Element_t	*ep;

    if (!e) return 0;
    if (!name) name = "TITLE";			/* useful default */

    for (; e->parent; e=e->parent)		/* move up tree */
	for (i=0; i<=e->my_eorder; i++) {	/* check preceding sibs */
	    ep = e->parent;
	    if (!strcmp(name, ep->econt[i]->gi))
		return ep->econt[i]->ndcont ?
			ep->econt[i]->dcont[0] : "-empty-";
	}

    return NULL;
}

/* ______________________________________________________________________ */
/*  Expands escaped strings in the input buffer (things like tabs, newlines,
 *  octal characters - using C style escapes) if outputting the buffer to
 *  the specified fp.  If fp is NULL, we're only preparing the output
 *  for the interpreter so don't expand escaped strings.  The hat/anchor
 *  character forces that position to appear at the beginning of a line.
 *  The cursor position is kept track of (optionally) so that this can be
 *  done.
 *  Arguments:
 *	Pointer to element under consideration.
 *	FILE pointer of where to print.
 *	Flag saying whether or not to keep track of our position in the output
 *	  stream. (We want to when writing to a file, but not for stderr.)
 */

void
OutputString(
    char	*s,
    FILE	*fp,
    int		track_pos
)
{
    char	c = 0, *sdata, *cp;
    static int	char_pos;   /* remembers our character position */
    static int  interp_pos; /* like char_pos but when output is to interp */
    int         *ppos;      /* points to appropriate line position var */

    if (fp)
	ppos = &char_pos;   /* writing to file */
    else
	ppos = &interp_pos; /* buffer will be read by interpreter */

    if (!s) s = "^";	    /* no string - go to start of line */

    for ( ; *s; s++) {
	if (fp && (*s == '\\')) { /* don't expand for interpreter */
	    s++;
	    if (track_pos) (*ppos)++;
	    switch (*s) {
		default:	c = *s;		break;

		case 's':	c = ' ';	break;

		case 't':	c = TAB;	break;

		case 'n':	c = NL;		*ppos = 0;	break;

		case 'r':	c = CR;		*ppos = 0;	break;

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		    /* for octal numbers (C style) of the form \012 */
		    c = *s - '0';
		    for (s++; ((*s >= '0') && (*s <= '7')); s++)
			c = (c << 3) + (*s - '0');
		    s--;
		    break;

		case '|':		/* SDATA */
		    s++;		/* point past \| */
		    sdata = s;
		    /* find matching/closing \| */
		    cp = s;
		    while (*cp && *cp != '\\' && cp[1] != '|') cp++;
		    if (!*cp) break;

		    *cp = EOS;		/* terminate sdata string */
		    cp++;
		    s = cp;		/* s now points to | */

		    cp = LookupSDATA(sdata);
		    if (cp) OutputString(cp, fp, track_pos);
		    else {
			/* not found - output sdata thing in brackets */
			Putc('[', fp);
			FPuts(sdata, fp);
			Putc(']', fp);
		    }
		    c = 0;
		    break;
	    }
	}
	else {		/* not escaped - just pass the character */
	    c = *s;
	    /* If caller wants us to track position, see if it's an anchor
	     * (ie, align at a newline). */
	    if (track_pos) {
		if (c == ANCHOR) {
		    /* If we're already at the start of a line, don't do
		     * another newline. */
		    if (*ppos != 0) c = NL;
		    else c = 0;
		}
		else (*ppos)++;
		if (c == NL) *ppos = 0;
	    }
	    else if (c == ANCHOR) c = NL;
	}
	if (c) Putc(c, fp);
    }
}

/* ______________________________________________________________________ */
/* resets the output buffer
 */
void ClearOutputBuffer(void)
{
outputBuffer.current = outputBuffer.base;
}

/* ______________________________________________________________________ */
/* determines if there is already some text in the output buffer,
 * returns 1 if so, else 0
 */
int OutputBufferActive(void)
{
return (outputBuffer.current != outputBuffer.base);
}

/* ______________________________________________________________________ */
/* terminates output buffer with a null char and returns the buffer
 */
char *GetOutputBuffer(void)
{
if (CheckOutputBuffer(1))
    *(outputBuffer.current)++ = '\0';

return outputBuffer.base;
}

/* ______________________________________________________________________ */
/* insures that there's enough room in outputBuffer to hold a string
 * of the given length.
 * Arguments: the length of the string
 */
static int CheckOutputBuffer(
    int length
)
{
    char *oldBase;
    int   oldSize, incr = OBUF_INCR;

    while (length > incr) incr += OBUF_INCR;

    if ((outputBuffer.current - outputBuffer.base + length)
			>
		outputBuffer.size) {
	oldBase = outputBuffer.base;
	oldSize = outputBuffer.size;
	outputBuffer.size += incr;
	outputBuffer.base =
	    outputBuffer.base ?
		realloc(outputBuffer.base, outputBuffer.size) :
		malloc(outputBuffer.size);
	if (outputBuffer.base == NULL) {
	    outputBuffer.base = oldBase;
	    outputBuffer.size = oldSize;
	    return 0;
	}
	outputBuffer.current =
	    outputBuffer.base + (outputBuffer.current - oldBase);
    }

    return 1;
}


/* ______________________________________________________________________ */
/* local version of fflush(3S)
 *
 * special cases a FILE of NULL to simply return success
 *
 */
int FFlush(FILE *stream)
{
    if (stream) return fflush(stream);
    return 0;
}


/* ______________________________________________________________________ */
/* local version of putc(3S)
 *
 * special cases a FILE of NULL by working into a buffer for later
 * use by the interpreter
 *
 * extra special hack: call Tcl interpreter with the character; worry
 * about "stream" somo other time, we'll default to stdout
 */
int Putc(
    int c,
    FILE *stream
)
{
    int result;
    int j;
    char *pc;
    char *tcl_str;
    Tcl_DString tcl_dstr;
    Tcl_Encoding tcl_enc;
    static int i = 0;
    static char argBuf[8];
    static char commandBuf[] = "OutputString \"                 ";

    if (stream) {
	argBuf[i++] = c;

	mblen(NULL, 0);

	if (mblen(argBuf, i) == -1) {
	    if (i < MB_CUR_MAX) {
		return c;
	    }
	    else {
		i = 0;
		fprintf(stderr,
		    "An invalid multi-byte character was found in the input.");
		return EOF;
	    }
	}

	pc = &(commandBuf[14]);
	switch (c) { /* escape those things that throw off tcl */
	    case '{':
	    case '}':
	    case '"':
	    case '\'':
	    case '[':
	    case ']':
	    case '$':
	    case '\\':
		*pc++ = '\\';
	}
	for (j = 0; j < i; ++j) *pc++ = argBuf[j]; i = 0;
	*pc++ = '"';
	*pc++ = 0;
	tcl_enc = Tcl_GetEncoding(NULL, NULL);
	tcl_str = Tcl_ExternalToUtfDString(tcl_enc, commandBuf, -1, &tcl_dstr);
	result = Tcl_Eval(interpreter, tcl_str);
	Tcl_DStringFree(&tcl_dstr);

	if (result != TCL_OK) {
	    fprintf(stderr,
		    "interpreter error \"%s\" at line %d executing:\n",
		    Tcl_GetStringResult(interpreter),
		    Tcl_GetErrorLine(interpreter));
	    fprintf(stderr, "\"%s\"\n", commandBuf);
	    return EOF;
	}
	return c;
    }

    if ((CheckOutputBuffer(1)) == 0)
	return EOF; /* out of space and can't grow the buffer */

    *(outputBuffer.current)++ = (char) c;

    return c;
}

/* ______________________________________________________________________ */
/* local version of fputs(3S)
 *
 * special cases a FILE of NULL by working into a buffer for later
 * use by the interpreter
 */
int FPuts(
    const char *s,
    FILE *stream
)
{
    static char commandBuf[128] = "OutputString \"";
    char *pBuff,*pb;
    const char *ps;
    int sLength;
    int result;
    char *tcl_str;
    Tcl_DString tcl_dstr;
    Tcl_Encoding tcl_enc;

    if ((sLength = strlen(s)) == 0)
	return 0; /* no need to call CheckOutputBuffer() */

    if (stream) {
	if (sLength > 100/2) { /* assume that every char must be escaped */
	    pBuff = malloc(sLength + 14 + 1);
	    commandBuf[14] = 0;
	    strcpy(pBuff, commandBuf);
	} else
	    pBuff = commandBuf;
	ps = s;
	pb = pBuff + 14;
	do {
	    switch (*ps) { /* escape those things that throw off Tcl */
		case '{':
		case '}':
		case '"':
		case '\'':
		case '[':
		case ']':
		case '\\':
		    *pb++ = '\\';
	    }
	    *pb++ = *ps++;
	} while (*ps);
	*pb++ = '"';
	*pb = 0;
	tcl_enc = Tcl_GetEncoding(NULL, NULL);
	tcl_str = Tcl_ExternalToUtfDString(tcl_enc, pBuff, -1, &tcl_dstr);
	result = Tcl_Eval(interpreter, tcl_str);
	Tcl_DStringFree(&tcl_dstr);

	if (result != TCL_OK) {
	    fprintf(stderr,
		    "interpreter error \"%s\" at line %d executing:\n",
		    Tcl_GetStringResult(interpreter),
                    Tcl_GetErrorLine(interpreter));
	    fprintf(stderr, "\"%s\"\n", pBuff);
	    if (pBuff != commandBuf) free(pBuff);
	    return EOF;
	}
	if (pBuff != commandBuf) free(pBuff);
	return 0;
    }

    if ((CheckOutputBuffer(sLength)) == 0)
	return EOF; /* out of space and can't grow the buffer */

    strncpy(outputBuffer.current, s, sLength);
    outputBuffer.current += sLength;

    return sLength; /* arbitrary non-negative number */
}

/* ______________________________________________________________________ */
/* Figure out value of SDATA entity.
 * We rememeber lookup hits in a "cache" (a shorter list), and look in
 * cache before general list.  Typically there will be LOTS of entries
 * in the general list and only a handful in the hit list.  Often, if an
 * entity is used once, it'll be used again.
 *  Arguments:
 *	Pointer to SDATA entity token in ESIS.
 *  Return:
 *	Mapped value of the SDATA entity.
 */

static char *
LookupSDATA(
    char	*s
)
{
    char	*v;
    static Map_t *Hits;		/* remember lookup hits */

    /* SDL SDL SDL SDL --- special (i.e., hack); see below      */
    /* we're going to replace the "123456" below with the SDATA */
    /*                         0123456789 012                   */
    static char spcString[] = "<SPC NAME=\"[123456]\">\0";
    static char spc[sizeof(spcString)];


    /* If we have a hit list, check it. */
    if (Hits) {
	if ((v = FindMappingVal(Hits, s))) return v;
    }

    v = FindMappingVal(SDATAmap, s);

    /* If mapping found, remember it, then return it. */
    if ((v = FindMappingVal(SDATAmap, s))) {
	if (!Hits) Hits = NewMap(IMS_sdatacache);
	SetMappingNV(Hits, s, v);
	return v;
    }

    /* SDL SDL SDL SDL --- special (i.e., hack)
       Special case sdata values of six letters surrounded by square
       brackets.  Just convert them over to the SDL <SPC> stuff
    */
    if ((strlen(s) == 8) &&
	(s[0] == '[')    &&
	(s[7] == ']')) {
	if (strcmp(s, "[newlin]") == 0) {
	    return "&\n";
	} else {
	    strcpy(spc, spcString);
	    strncpy(spc+12, s+1, 6);
	    return spc;
	}
    }

    fprintf(stderr, "Error: Could not find SDATA substitution '%s'.\n", s);
    return NULL;
}

/* ______________________________________________________________________ */
/*  Add tag 'name' of length 'len' to list of tag names (if not there).
 *  This is a list of null-terminated strings so that we don't have to
 *  keep using the name length.
 *  Arguments:
 *	Pointer to element name (GI) to remember.
 *  Return:
 *	Pointer to the SAVED element name (GI).
 */

char *
AddElemName(
    char	*name
)
{
    int		i;
    static int	n_alloc=0;	/* number of slots allocated so far */

    /* See if it's already in the list. */
    for (i=0; i<nUsedElem; i++)
	if (UsedElem[i][0] == name[0] && !strcmp(UsedElem[i], name))
	    return UsedElem[i];

    /* Allocate slots in blocks of N, so we don't have to call malloc
     * so many times. */
    if (n_alloc == 0) {
	n_alloc = IMS_elemnames;
	Calloc(n_alloc, UsedElem, char *);
    }
    else if (nUsedElem >= n_alloc) {
	n_alloc += IMS_elemnames;
	Realloc(n_alloc, UsedElem, char *);
    }
    UsedElem[nUsedElem] = strdup(name);
    return UsedElem[nUsedElem++];
}
/* ______________________________________________________________________ */
/*  Add attrib name to list of attrib names (if not there).
 *  This is a list of null-terminated strings so that we don't have to
 *  keep using the name length.
 *  Arguments:
 *	Pointer to attr name to remember.
 *  Return:
 *	Pointer to the SAVED attr name.
 */

char *
AddAttName(
    char	*name
)
{
    int		i;
    static int	n_alloc=0;	/* number of slots allocated so far */

    /* See if it's already in the list. */
    for (i=0; i<nUsedAtt; i++)
	if (UsedAtt[i][0] == name[0] && !strcmp(UsedAtt[i], name))
	    return UsedAtt[i];

    /* Allocate slots in blocks of N, so we don't have to call malloc
     * so many times. */
    if (n_alloc == 0) {
	n_alloc = IMS_attnames;
	Calloc(n_alloc, UsedAtt, char *);
    }
    else if (nUsedAtt >= n_alloc) {
	n_alloc += IMS_attnames;
	Realloc(n_alloc, UsedAtt, char *);
    }
    UsedAtt[nUsedAtt] = strdup(name);
    return UsedAtt[nUsedAtt++];
}

/* ______________________________________________________________________ */
/*  Find an element's attribute value given element pointer and attr name.
 *  Typical use: 
 *	a=FindAttByName("TYPE", t); 
 *	do something with a->val;
 *  Arguments:
 *	Pointer to element under consideration.
 *	Pointer to attribute name.
 *  Return:
 *	Pointer to the value of the attribute.
 */

/*
Mapping_t *
FindAttByName(
    Element_t	*e,
    char	*name
)
{
    int		i;
    if (!e) return NULL;
    for (i=0; i<e->natts; i++)
	if (e->atts[i].name[0] == name[0] && !strcmp(e->atts[i].name, name))
		return &(e->atts[i]);
    return NULL;
}
*/

char *
FindAttValByName(
    Element_t	*e,
    char	*name
)
{
    int		i;
    if (!e) return NULL;
    for (i=0; i<e->natts; i++)
	if (e->atts[i].name[0] == name[0] && !strcmp(e->atts[i].name, name))
	    return e->atts[i].sval;
    return NULL;
}

/* ______________________________________________________________________ */
/*  Find context of a tag, 'levels' levels up the tree.
 *  Space for string is passed by caller.
 *  Arguments:
 *	Pointer to element under consideration.
 *	Number of levels to look up tree.
 *	String to write path into (provided by caller).
 *  Return:
 *	Pointer to the provided string (for convenience of caller).
 */

char *
FindContext(
    Element_t	*e,
    int		levels,
    char	*con
)
{
    char	*s;
    Element_t	*ep;
    int		i;

    if (!e) return NULL;
    s = con;
    *s = EOS;
    for (i=0,ep=e->parent; ep && levels; ep=ep->parent,i++,levels--) {
	if (i != 0) *s++ = ' ';
	strcpy(s, ep->gi);
	s += strlen(s);
    }
    return con;
}


/* ______________________________________________________________________ */
/*  Tests relationship (specified by argument/flag) between given element
 *  (structure pointer) and named element.
 *  Returns pointer to matching tag if found, null otherwise.
 *  Arguments:
 *	Pointer to element under consideration.
 *	Pointer to name of elem whose relationsip we are trying to determine.
 *	Relationship we are testing.
 *  Return:
 *	Pointer to the provided string (for convenience of caller).
 */

Element_t *
QRelation(
    Element_t	*e,
    char	*s,
    Relation_t	rel
)
{
    int		i;
    Element_t	*ep;

    if (!e) return 0;

    /* we'll call e the "given element" */
    switch (rel)
    {
	case REL_Parent:
	    if (!e->parent || !e->parent->gi) return 0;
	    if (!strcmp(e->parent->gi, s)) return e->parent;
	    break;
	case REL_Child:
	    for (i=0; i<e->necont; i++)
		if (!strcmp(s, e->econt[i]->gi)) return e->econt[i];
	    break;
	case REL_Ancestor:
	    if (!e->parent || !e->parent->gi) return 0;
	    for (ep=e->parent; ep; ep=ep->parent)
		if (!strcmp(ep->gi, s)) return ep;
	    break;
	case REL_Descendant:
	    if (e->necont == 0) return 0;
	    /* check immediate children first */
	    for (i=0; i<e->necont; i++)
		if (!strcmp(s, e->econt[i]->gi)) return e->econt[i];
	    /* then children's children (recursively) */
	    for (i=0; i<e->necont; i++)
		if ((ep=QRelation(e->econt[i], s, REL_Descendant)))
		    return ep;
	    break;
	case REL_Sibling:
	    if (!e->parent) return 0;
	    ep = e->parent;
	    for (i=0; i<ep->necont; i++)
		if (!strcmp(s, ep->econt[i]->gi) && i != e->my_eorder)
		    return ep->econt[i];
	    break;
	case REL_Preceding:
	    if (!e->parent || e->my_eorder == 0) return 0;
	    ep = e->parent;
	    for (i=0; i<e->my_eorder; i++)
		if (!strcmp(s, ep->econt[i]->gi)) return ep->econt[i];
	    break;
	case REL_ImmPreceding:
	    if (!e->parent || e->my_eorder == 0) return 0;
	    ep = e->parent->econt[e->my_eorder-1];
	    if (!strcmp(s, ep->gi)) return ep;
	    break;
	case REL_Following:
	    if (!e->parent || e->my_eorder == (e->parent->necont-1))
		return 0;	/* last? */
	    ep = e->parent;
	    for (i=(e->my_eorder+1); i<ep->necont; i++)
		if (!strcmp(s, ep->econt[i]->gi)) return ep->econt[i];
	    break;
	case REL_ImmFollowing:
	    if (!e->parent || e->my_eorder == (e->parent->necont-1))
		return 0;	/* last? */
	    ep = e->parent->econt[e->my_eorder+1];
	    if (!strcmp(s, ep->gi)) return ep;
	    break;
	case REL_Cousin:
	    if (!e->parent) return 0;
	    /* Now, see if element's parent has that thing as a child. */
	    return QRelation(e->parent, s, REL_Child);
	    break;
	case REL_Is1stContent:
	    /* first confirm that our parent is an "s" */
	    if (!(ep = QRelation(e, s, REL_Parent))) return 0;
	    /* then check that we are the first content in that parent */
	    if ((ep->cont->type == CMD_OPEN) && (ep->cont->ch.elem == e))
		return ep;
	    break;
	case REL_HasOnlyContent:
	    /* first confirm that we have a child of "s" */
	    if (!(ep = QRelation(e, s, REL_Child))) return 0;
	    /* then check that it is our only content */
	    if (e->ncont == 1) return 0;
	    break;
	case REL_None:
	case REL_Unknown:
	    fprintf(stderr, "You cannot query 'REL_None' or 'REL_Unknown'.\n");
	    break;
    }
    return NULL;
}

/*  Given a relationship name (string), determine enum symbol for it.
 *  Arguments:
 *	Pointer to relationship name.
 *  Return:
 *	Relation_t enum.
 */
Relation_t
FindRelByName(
    char	*relname
)
{
    if (!strcmp(relname, "?")) {
	fprintf(stderr, "Supported query/relationships %s\n%s\n%s.\n", 
	    "child, parent, ancestor, descendant,",
	    "sibling, sibling+, sibling+1, sibling-, sibling-1,",
	    "cousin, isfirstcon, hasonlycon");
	return REL_None;
    }
    else if (StrEq(relname, "child"))		return REL_Child;
    else if (StrEq(relname, "parent"))		return REL_Parent;
    else if (StrEq(relname, "ancestor"))	return REL_Ancestor;
    else if (StrEq(relname, "descendant"))	return REL_Descendant;
    else if (StrEq(relname, "sibling"))		return REL_Sibling;
    else if (StrEq(relname, "sibling-"))	return REL_Preceding;
    else if (StrEq(relname, "sibling-1"))	return REL_ImmPreceding;
    else if (StrEq(relname, "sibling+"))	return REL_Following;
    else if (StrEq(relname, "sibling+1"))	return REL_ImmFollowing;
    else if (StrEq(relname, "cousin"))		return REL_Cousin;
    else if (StrEq(relname, "isfirstcon"))	return REL_Is1stContent;
    else if (StrEq(relname, "hasonlycon"))	return REL_HasOnlyContent;
    else fprintf(stderr, "Unknown relationship: %s\n", relname);
    return REL_Unknown;
}

/* ______________________________________________________________________ */
/*  This will descend the element tree in-order. (enter_f)() is called
 *  upon entering the node.  Then all children (data and child elements)
 *  are operated on, calling either DescendTree() with a pointer to
 *  the child element or (data_f)() for each non-element child node.
 *  Before leaving the node (ascending), (leave_f)() is called.  enter_f
 *  and leave_f are passed a pointer to this node and data_f is passed
 *  a pointer to the data/content (which includes the data itself and
 *  type information).  dp is an opaque pointer to any data the caller
 *  wants to pass.
 *  Arguments:
 *	Pointer to element under consideration.
 *	Pointer to procedure to call when entering element.
 *	Pointer to procedure to call when leaving element.
 *	Pointer to procedure to call for each "chunk" of content data.
 *	Void data pointer, passed to the avobe 3 procedures.
 */

void
DescendTree(
    Element_t	*e,
    void	(*enter_f)(),
    void	(*leave_f)(),
    void	(*data_f)(),
    void	*dp
)
{
    int		i;
    if (enter_f) (enter_f)(e, dp);
    for (i=0; i<e->ncont; i++) {
	if (e->cont[i].type == CMD_OPEN)
	    DescendTree(e->cont[i].ch.elem, enter_f, leave_f, data_f, dp);
	else
	    if (data_f) (data_f)(&e->cont[i], dp);
    }
    if (leave_f) (leave_f)(e, dp);
}

/* ______________________________________________________________________ */
/*  Add element, 'e', whose ID is 'idval', to a list of IDs.
 *  This makes it easier to find an element by ID later.
 *  Arguments:
 *	Pointer to element under consideration.
 *	Element's ID attribute value (a string).
 */

void
AddID(
    Element_t	*e,
    char	*idval
)
{
    static ID_t	*id_last;

    if (!IDList) {
	Malloc(1, id_last, ID_t);
	IDList = id_last;
    }
    else {
	Malloc(1, id_last->next, ID_t);
	id_last = id_last->next;
    }
    id_last->elem = e;
    id_last->id   = idval;
}

/* ______________________________________________________________________ */
/*  Return pointer to element who's ID is given.
 *  Arguments:
 *	Element's ID attribute value (a string).
 *  Return:
 *	Pointer to element whose ID matches.
 */

Element_t *
FindElemByID(
    char	*idval
)
{
    ID_t	*id;
    for (id=IDList; id; id=id->next)
	if (id->id[0] == idval[0] && !strcmp(id->id, idval)) return id->elem;
    return 0;
}

/* ______________________________________________________________________ */

