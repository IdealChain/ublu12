/* codegen_label.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

/* all known labels */
static struct label *labels;

/* last label in list */
static struct label *last_label;

/* counter to make outputted label names unique */
static unsigned int l_counter = 1;

struct label *label_get(struct sym *lab)
{
	assert(lab);
	
	/* find existing label */
	struct label *l = labels;
	while (l) {
		if (l->symbol == lab)
			return l;
		
		l = l->next;
	}
	
	/* create new one */
	l = label_anon();
	l->symbol = lab;
	
	return l;
}

struct label *label_anon()
{
	struct label *l = malloc(sizeof(*l));
	MALLOC_CHECK(l)
	
	l->nr = l_counter++;
	l->symbol = NULL;
	l->next = NULL;
	
	/* add to list */
	if (last_label) {
		last_label->next = l;
		last_label = l;
	} else {
		labels = last_label = l;
	}
	
	return l;
}
