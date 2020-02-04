/*
    Copyright (C) 2014 Parrot SA
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file ihm.c
 * @brief This file contains sources about ncurses IHM used by arsdk example "JumpingSumoSample"
 * @date 15/01/2015
 */

/*****************************************
 *
 *             include file :
 *
 *****************************************/

#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include <libARSAL/ARSAL.h>

#include "ihm.h"

/*****************************************
 *
 *             define :
 *
 *****************************************/

#define HEADER_X 0
#define HEADER_Y 0

#define INFO_X 0
#define INFO_Y 2

#define BATTERY_X 0
#define BATTERY_Y 4

/*****************************************
 *
 *             private header:
 *
 ****************************************/
void *IHM_InputProcessing(void *data);
void *IHM_ReadFile(void *data);
int flag = 0;
int quit = 0;
pthread_mutex_t flag_mutex;
pthread_cond_t cond;
/*****************************************
 *
 *             implementation :
 *
 *****************************************/

IHM_t *IHM_New (IHM_onInputEvent_t onInputEventCallback)
{
    int failed = 0;
    IHM_t *newIHM = NULL;
    pthread_mutex_init(&flag_mutex,NULL);
    pthread_cond_init(&cond, NULL);
    
    // check parameters
    if (onInputEventCallback == NULL)
    {
        failed = 1;
    }
    
    if (!failed)
    {
        //  Initialize IHM
        newIHM = malloc(sizeof(IHM_t));
        if (newIHM != NULL)
        {
            //  Initialize ncurses
            newIHM->mainWindow = initscr();
            newIHM->inputThread = NULL;
	    newIHM->readThread = NULL;
            newIHM->run = 1;
            newIHM->onInputEventCallback = onInputEventCallback;
            newIHM->customData = NULL;
        }
        else
        {
            failed = 1;
        }
    }
    
    if (!failed)
    {
        raw();                  // Line buffering disabled
        keypad(stdscr, TRUE);
        noecho();               // Don't echo() while we do getch
        timeout(100);
        
        refresh();
    }
    
    if (!failed)
    {
        //start input thread
        if(ARSAL_Thread_Create(&(newIHM->inputThread), IHM_InputProcessing, newIHM) != 0)
        {
            failed = 1;
        }
	if(ARSAL_Thread_Create(&(newIHM->readThread), IHM_ReadFile, newIHM) != 0)
        {
            failed = 1;
        }
    }
    
    if (failed)
    {
        IHM_Delete (&newIHM);
    }

    return  newIHM;
}

void IHM_Delete (IHM_t **ihm)
{
    //  Clean up

    if (ihm != NULL)
    {
        if ((*ihm) != NULL)
        {
            (*ihm)->run = 0;
            
            if ((*ihm)->inputThread != NULL)
            {
                ARSAL_Thread_Join((*ihm)->inputThread, NULL);
                ARSAL_Thread_Destroy(&((*ihm)->inputThread));
                pthread_mutex_destroy(&flag_mutex);
                pthread_cond_destroy(&cond);
                (*ihm)->inputThread = NULL;
            }
	    
	    if ((*ihm)->readThread != NULL)
            {
		quit = 1;
		ARSAL_Thread_Join((*ihm)->readThread, NULL);
		ARSAL_Thread_Destroy(&((*ihm)->readThread));
		(*ihm)->readThread = NULL;
            }
            
            delwin((*ihm)->mainWindow);
            (*ihm)->mainWindow = NULL;
            endwin();
            refresh();
            
            free (*ihm);
            (*ihm) = NULL;
        }
    }
}

void IHM_setCustomData(IHM_t *ihm, void *customData)
{
    if (ihm != NULL)
    {
        ihm->customData = customData;
    }
}

void *IHM_InputProcessing(void *data)
{
    IHM_t *ihm = (IHM_t *) data;
    int key = 0;
    
    if (ihm != NULL)
    {
        while (ihm->run)
        {
            key = getch();            
            if ((key == 27) || (key =='q'))
            {
		        flag = 0;
                if(ihm->onInputEventCallback != NULL)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_EXIT, ihm->customData);
                }
            }
            else if(key == KEY_UP)
            {
		        flag = 0;
                if(ihm->onInputEventCallback != NULL)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_FORWARD, ihm->customData);
                }
            }
            else if(key == KEY_DOWN)
            {
		        flag = 0;
                if(ihm->onInputEventCallback != NULL)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_BACK, ihm->customData);
                }
            }
            else if(key == KEY_LEFT)
            {
	       	    flag = 0;
                if(ihm->onInputEventCallback != NULL)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_LEFT, ihm->customData);
                }
            }
            else if(key == KEY_RIGHT)
            {
		        flag = 0;
                if(ihm->onInputEventCallback != NULL)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_RIGHT, ihm->customData);
                }
            }
            else if(key == ' ')
            {
		        flag = 0;
                if(ihm->onInputEventCallback != NULL)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_JUMP, ihm->customData);
                }
            }
	    else if(key == 'r' || key == 'R' || key == 114 || key == 82)
            {
                pthread_mutex_lock(&flag_mutex);
                flag = 1;
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&flag_mutex);
            }
            else
            {
                if(ihm->onInputEventCallback != NULL && !flag)
                {
                    ihm->onInputEventCallback (IHM_INPUT_EVENT_NONE, ihm->customData);
                }
            }
            
            usleep(10);
        }
    }
    
    return NULL;
}

void *IHM_ReadFile(void *data)
{
	char ch;
    // char file_name[25];
   	FILE *fp;
	int movements = 0;
	IHM_t *ihm = (IHM_t *) data; 
	eIHM_INPUT_EVENT events[20];
	int error = 0;

   	//printf("Enter name of a file you wish to see\n");
   	//gets(file_name);

   	fp = fopen("movements.txt", "r"); // read mode

   	if (fp == NULL)
   	{
	  perror("Error while opening the file.\n");
	  exit(EXIT_FAILURE);
      	  error = 1;
   	}

   	while((ch = fgetc(fp)) != EOF && movements < 20 && !error)
	{
      	  switch(ch)
	  {
	    case 'f':
              events[movements] = IHM_INPUT_EVENT_FORWARD;
	           movements++;
              break;
	    case 'b':
              events[movements] = IHM_INPUT_EVENT_BACK;
	           movements++;
              break;
	    case 'l':
              events[movements] = IHM_INPUT_EVENT_LEFT;
	           movements++;
              break;
	    case 'r':
              events[movements] = IHM_INPUT_EVENT_RIGHT;
	           movements++;
              break;
	    case 'j':
              events[movements] = IHM_INPUT_EVENT_JUMP;
	           movements++;
              break;
          case 'q':
              events[movements] = IHM_INPUT_EVENT_EXIT;
	           movements++;
	           break;
          }
	  
	}

   	fclose(fp);

	
	   
    while(!quit)
    {  
        pthread_mutex_lock(&flag_mutex);
        pthread_cond_wait(&cond, &flag_mutex);
    	pthread_mutex_unlock(&flag_mutex);
       	
	if(error)
	{
	  printf("No file found");
	}
	else
	{
        	for(int i = 0; i < movements; i++)
        	{	 
        		if(flag)
                {
                        if(events[i] == IHM_INPUT_EVENT_JUMP || events[i] == IHM_INPUT_EVENT_EXIT)
                        {
                            ihm->onInputEventCallback (events[i], ihm->customData);
                        }
                        else
                        {
                                    for(int duration = 0; duration < 30; duration++)
                                    {
                                        ihm->onInputEventCallback (events[i], ihm->customData);
                                    }
                        }
        		}
        		else
        		{
        			break;
        		}
            		usleep(2000000);
    		}
       }
        flag = 0;
    }
    
	return NULL;
}

void IHM_PrintHeader(IHM_t *ihm, char *headerStr)
{
    if (ihm != NULL)
    {
        move(HEADER_Y, 0);   // move to begining of line
        clrtoeol();          // clear line
        mvprintw(HEADER_Y, HEADER_X, headerStr);
    }
}

void IHM_PrintInfo(IHM_t *ihm, char *infoStr)
{
    if (ihm != NULL)
    {
        move(INFO_Y, 0);    // move to begining of line
        clrtoeol();         // clear line
        mvprintw(INFO_Y, INFO_X, infoStr);
    }
}

void IHM_PrintBattery(IHM_t *ihm, uint8_t percent)
{
    if (ihm != NULL)
    {
        move(BATTERY_Y, 0);     // move to begining of line
        clrtoeol();             // clear line
        mvprintw(BATTERY_Y, BATTERY_X, "Battery: %d", percent);
    }
}

