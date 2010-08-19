#include <input/input.h>
#include <stdlib.h>

input_listener_t* g_listenerList = 0;

void initializeInputCallbacks()
{
	g_listenerList = 0;

	//init_mouse(); //TODO: Fix mouse driver
	initializeKeyboard();

}

void sendInputMessage(uint32 device, uint32 main, void* additional)
{
	if (g_listenerList)
	{
		input_listener_t* iterator = g_listenerList;
		
		//Loop through the callbacks		
		while (iterator != 0)
		{
			if ((iterator->m_deviceid == -1) || (iterator->m_deviceid == device)) //If this callback should be triggered
			{
				if (iterator->m_callback)
					iterator->m_callback(device, main, additional); //Call it.
			}

			iterator = iterator->m_next;
		}
	}

return;
}

void registerInputListener(uint32 device, input_listener_callback cb)
{
	if (g_listenerList)
	{
		//Add a new entry to the end of the listener list
		input_listener_t* iterator = g_listenerList;

		//Find the last entry on the list
		while (iterator->m_next != 0) { iterator = iterator->m_next; }

		//Make the new entry
		input_listener_t* new_listener = (input_listener_t*) malloc(sizeof(input_listener_t));
		memset((void*)new_listener, 0, sizeof(input_listener_t));
		new_listener->m_deviceid = device;
		new_listener->m_callback = cb;
		
		//Set iterators next to new_listener
		iterator->m_next = new_listener;
	}
	else //No existing listeners yet
	{
		g_listenerList = (input_listener_t*) malloc(sizeof(input_listener_t));
		memset((void*)g_listenerList, 0, sizeof(g_listenerList));
		g_listenerList->m_deviceid = device;
		g_listenerList->m_callback = cb;
	}
}
