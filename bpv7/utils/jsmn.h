/*
 * MIT License
 *
 * Copyright (c) 2010 Serge Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef JSMN_H
#define JSMN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define JSMN_STRICT
#define JSMN_PARENT_LINKS

#ifdef JSMN_STATIC
#define JSMN_API static
#else
#define JSMN_API extern
#endif

  /**
   * JSON type identifier. Basic types are:
   * 	o Object
   * 	o Array
   * 	o String
   * 	o Other primitive: number, boolean (true/false) or null
   */
  typedef enum
  {
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT = 1,
    JSMN_ARRAY = 2,
    JSMN_STRING = 3,
    JSMN_PRIMITIVE = 4
  } jsmntype_t;

  enum jsmnerr
  {
    /* Not enough tokens were provided */
    JSMN_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3
  };

  /**
   * JSON token description.
   * type		type (object, array, string etc.)
   * start	start position in JSON data string
   * end		end position in JSON data string
   */
  typedef struct jsmntok
  {
    jsmntype_t type;
    int start;
    int end;
    int size;
#ifdef JSMN_PARENT_LINKS
    int parent;
#endif
  } jsmntok_t;

  /**
   * JSON parser. Contains an array of token blocks available. Also stores
   * the string being parsed now and current position in that string.
   */
  typedef struct jsmn_parser
  {
    unsigned int pos;     /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper;         /* superior token node, e.g. parent object or array */
  } jsmn_parser;

  /**
   * Create JSON parser over an array of tokens
   */
  JSMN_API void jsmn_init(jsmn_parser *parser);

  /**
   * Run JSON parser. It parses a JSON data string into and array of tokens, each
   * describing
   * a single JSON object.
   */
  JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                          jsmntok_t *tokens, const unsigned int num_tokens);

#ifndef JSMN_HEADER
  /**
   * Allocates a fresh unused token from the token pool.
   */
  jsmntok_t *jsmn_alloc_token(jsmn_parser *parser, jsmntok_t *tokens,
                                     const size_t num_tokens);

  /**
   * Fills token type and boundaries.
   */
  void jsmn_fill_token(jsmntok_t *token, const jsmntype_t type,
                              const int start, const int end);

  /**
   * Fills next available token with JSON primitive.
   */
  int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
                                  const size_t len, jsmntok_t *tokens,
                                  const size_t num_tokens);
  /**
   * Fills next token with JSON string.
   */
  int jsmn_parse_string(jsmn_parser *parser, const char *js,
                               const size_t len, jsmntok_t *tokens,
                               const size_t num_tokens);
  /**
   * Parse JSON string and fill tokens.
   */
  JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                          jsmntok_t *tokens, const unsigned int num_tokens);
  /**
   * Creates a new parser based over a given buffer with an array of tokens
   * available.
   */
  JSMN_API void jsmn_init(jsmn_parser *parser);

#endif /* JSMN_HEADER */

#ifdef __cplusplus
}
#endif

#endif /* JSMN_H */
