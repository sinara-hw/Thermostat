//*****************************************************************************
//
// enet_fs.c - File System Processing for lwIP Web Server Apps.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/ssi.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
//#include "httpserver_raw/httpd.h"
#include "lwip/apps/httpd.h"
//#include "httpserver_raw/fs.h"
#include "lwip/apps/fs.h"
//#include "httpserver_raw/fsdata.h"
#include "http/fsdata.h"

#include "enet_fsdata.h"

//*****************************************************************************
//
// The number of milliseconds that has passed since the last disk_timerproc()
// call.
//
//*****************************************************************************
static uint32_t ui32TickCounter = 0;

//*****************************************************************************
//
// Initialize the file system.
//
//*****************************************************************************
void
fs_init(void)
{
}

//*****************************************************************************
//
// File System tick handler.
//
//*****************************************************************************
void
fs_tick(uint32_t ui32TickMS)
{
    //
    // Increment the tick counter.
    //
    ui32TickCounter += ui32TickMS;
}

//*****************************************************************************
//
// Open a file and return a handle to the file, if found.  Otherwise,
// return NULL.
//
//*****************************************************************************
err_t fs_open(struct fs_file *psFile, const char *pcName)
{
    const struct fsdata_file *psTree;
    err_t err;
//    struct fs_file *psFile = NULL;

    //
    // Allocate memory for the file system structure.
    //

    if(psFile == NULL)
    {
        err = ERR_MEM;
        return(err);
    }

    //
    // Initialize the file system tree pointer to the root of the linked list.
    //
    psTree = FS_ROOT;

    //
    // Begin processing the linked list, looking for the requested file name.
    //
    while(NULL != psTree)
    {
        //
        // Compare the requested file "name" to the file name in the
        // current node.
        //
        if(ustrncmp(pcName, (char *)psTree->name, psTree->len) == 0)
        {
            //
            // Fill in the data pointer and length values from the
            // linked list node.
            //
            psFile->data = (char *)psTree->data;
            psFile->len = psTree->len;

            //
            // For now, we setup the read index to the end of the file,
            // indicating that all data has been read.
            //
            psFile->index = psTree->len;

            //
            // We are not using any file system extensions in this
            // application, so set the pointer to NULL.
            //
            psFile->pextension = NULL;

            //
            // Exit the loop and return the file system pointer.
            //

            err = ERR_OK;

            break;
        }

        //
        // If we get here, we did not find the file at this node of the linked
        // list.  Get the next element in the list.
        //
        psTree = psTree->next;

        err = ERR_ARG;
    }

    //
    // If we didn't find the file, ptTee will be NULL.  Make sure we
    // return a NULL pointer if this happens.
    //
    if(psTree == NULL)
    {
        mem_free(psFile);
        psFile = NULL;
    }

    //
    // Return error variable.
    //

    return(err);
}

//*****************************************************************************
//
// Close an opened file designated by the handle.
//
//*****************************************************************************
void
fs_close(struct fs_file *psFile)
{
    //
    // Free the main file system object.
    //
    mem_free(psFile);
}

//*****************************************************************************
//
// Read the next chunck of data from the file.  Return the count of data
// that was read.  Return 0 if no data is currently available.  Return
// a -1 if at the end of file.
//
//*****************************************************************************
int
fs_read(struct fs_file *psFile, char *pcBuffer, int iCount)
{
    int iAvailable;

    //
    // Check to see if more data is available.
    //
    if(psFile->index == psFile->len)
    {
        //
        // There is no remaining data.  Return a -1 for EOF indication.
        //
        return(-1);
    }

    //
    // Determine how much data we can copy.  The minimum of the 'iCount'
    // parameter or the available data in the file system buffer.
    //
    iAvailable = psFile->len - psFile->index;
    if(iAvailable > iCount)
    {
        iAvailable = iCount;
    }

    //
    // Copy the data.
    //
    memcpy(pcBuffer, psFile->data + iAvailable, iAvailable);
    psFile->index += iAvailable;

    //
    // Return the count of data that we copied.
    //
    return(iAvailable);
}

//*****************************************************************************
//
// Determine the number of bytes left to read from the file.
//
//*****************************************************************************
int
fs_bytes_left(struct fs_file *psFile)
{
    //
    // Return the number of bytes left to be read from this file.
    //
    return(psFile->len - psFile->index);
}
