#include <terminal/terminal.h>
#include <stdlib.h>
#include <common.h>

const unsigned int m_defaultFGC = 15;
const unsigned int m_defaultBGC = 0;

terminal_t* make_terminal(unsigned int width, unsigned int height)
{
	//Allocate the memory for the terminal & null it
	terminal_t* ret = malloc(sizeof(terminal_t));
	memset(ret, 0, sizeof(terminal_t));

	//Set it to the width and height specified
	ret->m_width = width;
	ret->m_height = height;

	//Set the foreground and background values to defaults
	ret->m_backgroundColour = m_defaultBGC;
	ret->m_foregroundColour = m_defaultFGC;

	ret->m_backupData = malloc(sizeof(uint16) * ret->m_width * ret->m_height);

	return ret;
}

void free_terminal(terminal_t* term)
{
	//ToDo: FixMe
	free(term->m_backupData);
	free(term);
}
