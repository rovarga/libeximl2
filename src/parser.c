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

#include <stdbool.h>
#include <libxml/tree.h>

typedef enum {
	NO_DOCUMENT,
	DOCUMENT_ROOT,
	ELEMENT_START,
	ATTRIBUTE_START,
} parse_state_t;

struct attribute {
	struct attribute *prev;
	const xmlChar    *name;
	const xmlChar    *prefix;
	const xmlChar    *uri;
	xmlChar          *value; // Not in data!
	xmlChar          data[];
};

struct element {
	struct element *prev;
	const xmlChar  *name;
	const xmlChar  *prefix;
	const xmlChar  *uri;
	xmlChar        data[];
};

struct exi_state {
	xmlSAXHandlerPtr handler;
	void             *ctx;
	parse_state_t    state;

	struct element   *element;
	struct attribute *attr;
};

static errorCode exiFatalError(const errorCode code, const char *msg, void *ptr)
{
	struct exi_state *state = ptr;
	state->handler->fatalError(state->ctx, "%s", msg);
	return EXIP_OK;
}

static errorCode exiStartDocument(void *ptr)
{
	struct exi_state *state = ptr;

	printf("Start of document\n");
	assert(state->state == NO_DOCUMENT);
	state->handler->startDocument(state->ctx);
	state->state = DOCUMENT_ROOT;

	return EXIP_OK;
}

/*
 * Called whenever we may have finished gathering information for sending
 * out a start element SAX event.
 */
static errorCode emit_start_element(struct exi_state *state)
{
	if (state->state == ELEMENT_START) {
		int nr_attrs = 0;
		struct attribute *a = state->attr;
		while (a) {
			++nr_attrs;
			a = a->prev;
		}

		const xmlChar **attrs = malloc(5 * sizeof(struct xmlChar *) * nr_attrs);
		if (!attrs) {
			return EXIP_MEMORY_ALLOCATION_ERROR;
		}

		a = state->attr;
		for (int i = 0; i < nr_attrs; ++i) {
			attrs[5 * i] = a->name;
			attrs[5 * i + 1] = a->prefix;
			attrs[5 * i + 2] = a->uri;
			attrs[5 * i + 3] = a->value;
			attrs[5 * i + 4] = a->value + xmlStrlen(a->value);
		}

		state->handler->startElementNs(state->ctx,
			state->element->name,
			state->element->prefix,
			state->element->uri,
			0, NULL, nr_attrs, 0, attrs);
		free(attrs);

		a = state->attr;
		while (a) {
			struct attribute *p = a->prev;

			free(a->value);
			free(a);
			a = p;
		}
		state->attr = NULL;
	}

	return EXIP_OK;
}

static errorCode exiEndDocument(void *ptr)
{
	struct exi_state *state = ptr;

	printf("End of document reached\n");
	state->handler->endDocument(state->ctx);
	state->state = NO_DOCUMENT;

	return EXIP_OK;
}

static size_t push_string(xmlChar *dst, const xmlChar **where, const String *str)
{
	if (str) {
		memcpy(dst, str->str, str->length);
		dst[str->length] = 0;
		*where = dst;
		return str->length + 1;
	} else {
		*where = NULL;
		return 0;
	}
}

static size_t qname_size(const QName *qname)
{
	size_t ret = qname->localName->length + 1;

	if (qname->uri) {
		ret += qname->uri->length + 1;
	}
	if (qname->prefix) {
		ret += qname->prefix->length + 1;
	}

	return ret;
}

static errorCode exiStartElement(QName qname, void *ptr)
{
	struct exi_state *state = ptr;

	printf("Start of element\b");

	errorCode err = emit_start_element(state);
	if (err != EXIP_OK) {
		return err;
	}

	// Allocate enough memory
	struct element *e = malloc(sizeof(struct element) + qname_size(&qname));
	if (e == NULL) {
		return EXIP_MEMORY_ALLOCATION_ERROR;
	}

	// Copy data into the element
	size_t offset = 0;
	offset += push_string(e->data + 0, &e->name, qname.localName);
	offset += push_string(e->data + offset, &e->prefix, qname.prefix);
	offset += push_string(e->data + offset, &e->uri, qname.uri);

	e->prev = state->element;
	state->element = e;
	return EXIP_OK;
}

static errorCode exiEndElement(void *ptr)
{
	struct exi_state *state = ptr;

	printf("End of element\b");

	const errorCode err = emit_start_element(state);
	if (err != EXIP_OK) {
		return err;
	}

	struct element *tmp = state->element;
	if (!tmp) {
		fprintf(stderr, "End of element singalled when no element present");
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	state->handler->endElementNs(state->ctx, tmp->name, tmp->prefix, tmp->uri);
	state->element = tmp->prev;
	free(tmp);
	return EXIP_OK;
}

static errorCode exiAttribute(QName qname, void *ptr)
{
	struct exi_state *state = ptr;

	if (state->state != ELEMENT_START) {
		fprintf(stderr, "Encountered attribute in state %d", state->state);
		return EXIP_INCONSISTENT_PROC_STATE;
	}

	struct attribute *a = malloc(sizeof(struct attribute) + qname_size(&qname));
	if (!a) {
		return EXIP_MEMORY_ALLOCATION_ERROR;
	}

	// Copy data into the element
	size_t offset = 0;
	offset += push_string(a->data + 0, &a->name, qname.localName);
	offset += push_string(a->data + offset, &a->prefix, qname.prefix);
	offset += push_string(a->data + offset, &a->uri, qname.uri);

	// Push into state
	a->value = NULL;
	a->prev = state->attr;
	state->attr = a->prev;
	state->state = ATTRIBUTE_START;

	return EXIP_OK;
}

static errorCode exiStringData(const String value, void *ptr)
{
	struct exi_state *state = ptr;

	if (state->state == ATTRIBUTE_START) {
		if (state->attr == NULL) {
			fprintf(stderr, "String data encountered with no attribute present");
			return EXIP_INCONSISTENT_PROC_STATE;
		}
		if (state->attr->value != NULL) {
			fprintf(stderr, "Duplicate data encountered for attribute");
			return EXIP_INCONSISTENT_PROC_STATE;
		}

		xmlChar *tmp = malloc(value.length + 1);
		if (!tmp) {
			return EXIP_MEMORY_ALLOCATION_ERROR;
		}
		memcpy(tmp, value.str, value.length);
		tmp[value.length] = 0;

		state->attr->value = tmp;
		state->state = ELEMENT_START;
		return EXIP_OK;
	} else {
		state->handler->characters(state->ctx, BAD_CAST value.str, value.length);
		return EXIP_OK;
	}
}

static errorCode exiDecimalData(Decimal value, void *ptr)
{
	String str = { .length = 0, };
	errorCode err = decimalToString(value, &str);
	if (err == EXIP_OK) {
		err = exiStringData(str, ptr);
		EXIP_MFREE(str.str);
	}

	return err;
}

static errorCode exiIntData(Integer value, void *ptr)
{
	String str = { .length = 0, };
	errorCode err = integerToString(value, &str);
	if (err == EXIP_OK) {
		err = exiStringData(str, ptr);
		EXIP_MFREE(str.str);
	}

	return err;
}

static errorCode exiFloatData(Float value, void *ptr)
{
	String str = { .length = 0, };
	errorCode err = floatToString(value, &str);
	if (err == EXIP_OK) {
		err = exiStringData(str, ptr);
		EXIP_MFREE(str.str);
	}

	return err;
}

static errorCode exiBooleanData(boolean value, void *ptr)
{
	String str = { .length = 0, };
	errorCode err = booleanToString(value, &str);
	if (err == EXIP_OK) {
		err = exiStringData(str, ptr);
		EXIP_MFREE(str.str);
	}

	return err;
}

static errorCode exiDateTimeData(EXIPDateTime value, void *ptr)
{
	String str = { .length = 0, };
	errorCode err = dateTimeToString(value, &str);
	if (err == EXIP_OK) {
		err = exiStringData(str, ptr);
		EXIP_MFREE(str.str);
	}

	return err;
}

static errorCode exiBinaryData(const char* binary_val, Index nbytes, void *ptr)
{
	return EXIP_NOT_IMPLEMENTED_YET;
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

	struct exi_state state = { .state = NO_DOCUMENT, };
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
//		xmlFreeDoc(state.document);
		return NULL;
	}

	return NULL;
//	return state.document;
}

