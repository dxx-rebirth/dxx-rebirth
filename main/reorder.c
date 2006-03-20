/** new reorder by Victor Rachels (the old one was too inflexible)
    thanks to Arne de Bruijn for the original ver **/

#include <stdio.h>

#include "weapon.h"
#include "newmenu.h"
#include "text.h"
#include "key.h"
#include "reorder.h"

#define MAX_AUTOSELECT_ITEMS (MAX_PRIMARY_WEAPONS+NEWPRIMS > MAX_SECONDARY_WEAPONS+NEWSECS ? MAX_PRIMARY_WEAPONS+NEWPRIMS : MAX_SECONDARY_WEAPONS+NEWSECS)

int default_primary_order[MAX_PRIMARY_WEAPONS+NEWPRIMS] = { 1, 2, 3, 4, 5, -1, -2, -3, -4, -5, -6, -7, -8};
int default_secondary_order[MAX_SECONDARY_WEAPONS+NEWSECS] = { 1, 2, -1, 3, 4 };

int primary_order[MAX_PRIMARY_WEAPONS+NEWPRIMS];
int secondary_order[MAX_SECONDARY_WEAPONS+NEWSECS];

int order_items[MAX_AUTOSELECT_ITEMS+1];

int highest_primary=0;
int highest_secondary=0;

int LaserPowSelected=0;

void reorder_poll(int nitems, newmenu_item * menus, int *key, int citem)
{
 int swap = 0;
 char *text;
 int w;

   if (citem > 0 && (*key == KEY_UP + KEY_SHIFTED || *key == KEY_PAD8 + KEY_SHIFTED)) 
    swap = -1;

   if (citem < nitems - 1 && (*key == KEY_DOWN + KEY_SHIFTED || *key == KEY_PAD2 + KEY_SHIFTED)) 
    swap = 1;

   if (swap)
    {
      text = menus[citem].text;
      menus[citem].text = menus[citem + swap].text;
      menus[citem + swap].text = text;
      w = order_items[citem];
      order_items[citem] = order_items[citem + swap];
      order_items[citem + swap] = w;
      menus[citem].redraw = 1;
      menus[citem + swap].redraw = 1;
       if (swap > 0)
        *key = KEY_DOWN;
       else
        *key = KEY_UP;
    }
}

void reorder(int primaries, int *weapon_order, char **names)
{
 int i,j,n;
 char title[50];
 newmenu_item m[MAX_AUTOSELECT_ITEMS+1];

   if(primaries)
    {
     n=MAX_PRIMARY_WEAPONS + NEWPRIMS;
     sprintf(title,"Reorder primary\nShift+Up/Down to move item");
    }
   else //secondaries
    {
     n=MAX_SECONDARY_WEAPONS + NEWSECS;
     sprintf(title,"Reorder secondary\nShift+Up/Down to move item");
    }

  j=0;
   for(i=0 ; i<n ; i++)
    if(weapon_order[i] > j)
     j = weapon_order[i];

   for(i=0 ; i<n ; i++)
    order_items[j-weapon_order[i]] = i;
  order_items[j-0] = -1;

   for(i=0 ; i < n+1 ; i++)
    {
     j = order_items[i];
     m[i].type = NM_TYPE_MENU;

      if(primaries && j>=MAX_PRIMARY_WEAPONS)
       {
         switch(j-MAX_PRIMARY_WEAPONS)
          {
           case 0 : m[i].text = "Laser L1"; break;
           case 1 : m[i].text = "Laser L2"; break;
           case 2 : m[i].text = "Laser L3"; break;
           case 3 : m[i].text = "Laser L4"; break;
           case 4 : m[i].text = "Quad Laser L1"; break;
           case 5 : m[i].text = "Quad Laser L2"; break;
           case 6 : m[i].text = "Quad Laser L3"; break;
           case 7 : m[i].text = "Quad Laser L4"; break;
          }
       }
      else if(!primaries && j>=MAX_SECONDARY_WEAPONS) //secondaries
       {
         switch(j-MAX_SECONDARY_WEAPONS)
          {
          }
       }
      else if(j<0)
       m[i].text = "--- Never autoselect below ---";
      else
       m[i].text = names[j];
    }

  newmenu_do(NULL, title, n+1, m, reorder_poll);

   for(i=0; i<n+1 ; i++)
    if(order_items[i] < 0)
     {
      j=i;
      break;
     }
   for (i=0 ; i<n+1 ; i++)
    if(order_items[i] >= 0)
     weapon_order[order_items[i]] = j - i;

}

void reorder_primary()
{
 int i;

  reorder(1, primary_order, &PRIMARY_WEAPON_NAMES(0));

  highest_primary=0;
   for(i=0; i<(MAX_PRIMARY_WEAPONS+NEWPRIMS); i++)
    if(primary_order[i]>0)
     highest_primary++;
}


void reorder_secondary()
{
 int i;

  reorder(0, secondary_order, &SECONDARY_WEAPON_NAMES(0));

  highest_secondary=0;
   for(i=0; i<(MAX_SECONDARY_WEAPONS+NEWSECS); i++)
    if(secondary_order[i]>0)
     highest_secondary++;
}

