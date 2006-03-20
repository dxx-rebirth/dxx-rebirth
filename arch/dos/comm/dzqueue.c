/*
 * This file is part of DZComm.
 * Version : 0.6
 * General purpose FIFO queue functions set.
 * Copyright (c) 1997 Dim Zegebart, Moscow Russia.
 * zager@post.comstar.ru
 * http://www.geocities.com/siliconvalley/pines/7817
 * file : queue.c
 */

#include <stdlib.h> /* NULL */
#include "dzcomm.h"
#include "internal.h"

typedef unsigned int uint;
typedef unsigned short int usint;
typedef unsigned char byte;

#define QH(q) q->queue+q->head
#define QT(q) q->queue+q->tail

//------------------------- FIFO QUEUE -----------------------------
// 'first in first out' queue functions

inline void queue_reset(fifo_queue *q)
{ q->tail=0;
  q->head=0;
}
END_OF_FUNCTION(queue_reset);


int queue_resize(fifo_queue *q,uint new_size)
{ int *tmp;
  if ((tmp=(int*)realloc(q->queue,sizeof(int)*new_size))==NULL) return(0);

  if (new_size>q->size) q->resize_counter++;
  else if (new_size<=q->initial_size) q->resize_counter=0;
  else q->resize_counter--;

  q->queue=tmp;
  q->size=new_size;
  q->fill_level=3*(q->size>>2); // 3/4
  return(1);
}

//-------------------- QUEUE_DELETE --------------------------

void queue_delete(fifo_queue *q)
{ if (q==NULL) return;
  free(q->queue);
  free(q);
}

//--------------------- QUEUE_PUT ------------------------------

inline int queue_put(fifo_queue *q,int c)
{ return(queue_put_(q,&c));
}
END_OF_FUNCTION(queue_put);

int queue_put_(fifo_queue *q,void *c)
{ int n=0;

  if (!q) return(0);

  DISABLE();
  memcpy(QT(q),c,q->dsize);
  q->tail+=q->dsize;
  if (q->tail==q->head)
   { q->head+=q->dsize;
     if (q->head>(q->size-q->dsize)) q->head=0;
   }

  if(q->tail>(q->size-q->dsize)) q->tail=0;

  n=0;
  if (q->head<q->tail) {if ((q->tail-q->head)>=q->fill_level) n=1;}
  else
   if ((q->head-q->tail)<=q->size-q->fill_level) n=1;

  ENABLE();

  return(n);
}
END_OF_FUNCTION(queue_put_);

//---------------------- QUEUE GET ---------------------------

inline int queue_get(fifo_queue *q)
{ int c;
  queue_get_(q,&c);
  return(c);
}
END_OF_FUNCTION(queue_get);

int queue_get_(fifo_queue *q,void *c)
{
  int n;

  DISABLE();
  memcpy(c,QH(q),q->dsize);
  q->head+=q->dsize;

  if (q->head>(q->size-q->dsize)) q->head=0;
  if (q->head==q->tail) queue_reset(q);

  n=0;
  if (q->head<q->tail) {if ((q->tail-q->head)>=q->fill_level) n=1;}
  else
   if ((q->head-q->tail)<=q->size-q->fill_level) n=1;

  ENABLE();
  return(n);
}
END_OF_FUNCTION(queue_get_);
//---------------------- QUEUE EMPTY ---------------------------

inline int queue_empty(fifo_queue *q)
{ if (q==NULL) return(1);
  if (q->head==q->tail)
   { if (q->empty_handler!=NULL) return(-1);
     return(1);
   }
  return(0);
}
END_OF_FUNCTION(queue_empty);


//------------------------------- QUEUE NEW --------------------------

fifo_queue* queue_new(uint size)
{
  return(queue_new_(size,4));
}

//------------------------- QUEUE_NEW_ --------------------------

fifo_queue* queue_new_(uint size,uint dsize)
{ fifo_queue *q=NULL;

  if (dsize<=0||dsize>4) return(NULL);
  if ((q=(fifo_queue*)malloc(sizeof(fifo_queue)))==NULL) return(NULL);

  if (!size) size=1024; //if illegal size, turn it to default size
  if ((q->queue=malloc(sizeof(int)*size*dsize))==NULL) return(NULL);

  q->dsize=dsize;
  q->size=size*dsize;
  q->initial_size=size*dsize;//size;
  q->resize_counter=0;
  q->fill_level=3*q->size/4; // 3/4
  q->empty_handler=NULL;
  queue_reset(q);
  _go32_dpmi_lock_data(q->queue,sizeof(int)*size*dsize);
  _go32_dpmi_lock_data(q,sizeof(fifo_queue));

  return(q);
}

void lock_queue_functions(void)
{
  LOCK_FUNCTION(queue_empty);
  LOCK_FUNCTION(queue_put);
  LOCK_FUNCTION(queue_get);
  LOCK_FUNCTION(queue_put_);
  LOCK_FUNCTION(queue_get_);
  LOCK_FUNCTION(queue_reset);
}
