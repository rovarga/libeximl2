/*
 * libeximl2, a exip-to-libxml2 bridge library
 * Copyright (C) 2013  Robert Varga
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <EXIParser.h>
#include <stringManipulate.h>

#include <libxml/tree.h>

struct exi_state {
	xmlDocPtr document;
};

static errorCode exiFatalError(const errorCode code, const char *msg, void *app_data)
{

}

static errorCode exiStartDocument(void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiEndDocument(void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiStartElement(QName qname, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiEndElement(void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiAttribute(QName qname, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiStringData(const String value, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiDecimalData(Decimal value, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiIntData(Integer int_val, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiFloatData(Float fl_val, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiBooleanData(boolean bool_val, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiDateTimeData(EXIPDateTime dt_val, void *ptr)
{
	struct exi_state *state = ptr;

}

static errorCode exiBinaryData(const char* binary_val, Index nbytes, void *ptr)
{
	struct exi_state *state = ptr;

}

xmlDocPtr exi_parse(const void *ptr, size_t length)
{
        BinaryBuffer buffer = {
                .buf = (void *) ptr,
                .bufLen = length,
                .bufContent = length,
                .ioStrm = {
                        .readWriteToStream = NULL,
                        .stream = NULL,
                },
        };

	struct exi_state state = { .document = NULL, };
	Parser parser;
	errorCode err = initParser(&parser, buffer, &state);
	if (err != EXIP_OK) {
		fprintf(stderr, "Failed to initialize EXI parser: %s (%s:%d).", GET_ERR_STRING(err), __FILE__, __LINE__);
		return NULL;
	}

        parser.handler.fatalError = exiFatalError;
        parser.handler.error = exiFatalError;
        parser.handler.startDocument = exiStartDocument;
        parser.handler.endDocument = exiEndDocument;
        parser.handler.startElement = exiStartElement;
        parser.handler.attribute = exiAttribute;
        parser.handler.stringData = exiStringData;
        parser.handler.endElement = exiEndElement;
        parser.handler.decimalData = exiDecimalData;
        parser.handler.intData = exiIntData;
        parser.handler.floatData = exiFloatData;
        parser.handler.booleanData = exiBooleanData;
        parser.handler.dateTimeData = exiDateTimeData;
        parser.handler.binaryData = exiBinaryData;

	err = parseHeader(&parser, FALSE);
	if (err != EXIP_OK) {
		fprintf(stderr, "Failed to parse EXI header: %s (%s:%d).", GET_ERR_STRING(err), __FILE__, __LINE__);
		return NULL;
	}

	// FIXME: set the appropriate schema
	err = setSchema(&parser, NULL);
	if (err != EXIP_OK) {
		fprintf(stderr, "Failed to set EXI schema: %s (%s:%d).", GET_ERR_STRING(err), __FILE__, __LINE__);
		return NULL;
	}

	do {
		err = parseNext(&parser);
	} while (err == EXIP_OK);

	destroyParser(&parser);

	if (err != EXIP_PARSING_COMPLETE) {
		fprintf(stderr, "Failed to parse EXI stream: %s (%s:%d).", GET_ERR_STRING(err), __FILE__, __LINE__);
		xmlFreeDoc(state.document);
		return NULL;
	}

	return state.document;
}

