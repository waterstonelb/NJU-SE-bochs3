
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "keyboard.h"

PRIVATE void clean_screen();
PRIVATE void set_cursor(unsigned int);
PRIVATE void str_match();
PRIVATE void refresh_screen();
PRIVATE void back();

PRIVATE int time;
PRIVATE int ONLY;
PRIVATE int esc_mode;

PUBLIC char keys[80 * 25] = {0};
PUBLIC char s_keys[80 * 25] = {0};
PUBLIC int pos[80 * 25] = {0};
PUBLIC char delete[80 * 25] = {0};
PUBLIC int d_index;
PUBLIC int index;
PUBLIC int p_index;
PUBLIC int s_index;

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	clean_screen();
	esc_mode = 0;
	index = 0;
	ONLY=0;
	d_index = 0;
	s_index = 0;
	p_index = 0;
	time = get_ticks();
	while (1)
	{
		if (((get_ticks() - time) * 1000 / HZ) > 200000 && esc_mode == 0)
		{
			clean_screen();
			index = 0;
			d_index = 0;
			s_index = 0;
			p_index = 0;
			time = get_ticks();
		}
		keyboard_read();

		set_cursor(disp_pos);
	}
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(u32 key)
{
	char output[2] = {'\0', '\0'};
	if (esc_mode)
	{
		if(ONLY){
			if((key&MASK_RAW) == ESC){
				esc_mode = 0;
				s_index = 0;
				refresh_screen();
				return;
			}else{
				return;
			}
		}
		if (!(key & FLAG_EXT))
		{
			if (!(key & FLAG_CTRL_L || key & FLAG_CTRL_R))
			{
				output[0] = key & 0xFF;
				s_keys[s_index++] = output[0];
				disp_color_str(output, BLUE);
			}
		}
		else
		{
			int raw_code = key & MASK_RAW;
			switch (raw_code)
			{
			case ENTER:
				s_keys[s_index] = '\0';
				clean_screen();
				str_match();
				disp_color_str(s_keys, BLUE);
				ONLY=1;
				break;
			case TAB:
				s_keys[s_index++] = '\t';
				int num = 4 - (disp_pos / 2 % 4);
				for (int i = 0; i < num; i++)
					disp_str(" ");
				break;
			case BACKSPACE:
				if (s_index > 0)
				{
					disp_pos -= 2;
					disp_str(" ");
					disp_pos -= 2;
					s_keys[--s_index] = '\0';
				}
				break;
			case ESC:
				esc_mode = 0;
				s_index = 0;
				refresh_screen();
				break;
			default:
				break;
			}
		}
	}
	else
	{
		if (!(key & FLAG_EXT))
		{
			if ((key & FLAG_CTRL_L || key & FLAG_CTRL_R) && (key & 0xFF) == 'z')
			{
				back();
			}
			else
			{
				output[0] = key & 0xFF;
				keys[index] = output[0];
				pos[p_index++] = ++index;
				disp_str(output);
			}
		}
		else
		{
			int raw_code = key & MASK_RAW;
			switch (raw_code)
			{
			case ENTER:
				keys[index] = '\n';
				pos[p_index++] = ++index;
				disp_pos = 160 * (disp_pos / 160 + 1);
				set_cursor(disp_pos);
				break;
			case TAB:
				keys[index] = '\t';
				pos[p_index++] = ++index;
				int num = 4 - (disp_pos / 2 % 4);
				for (int i = 0; i < num; i++)
				{
					disp_str(" ");
				}
				break;
			case BACKSPACE:
				if (index > 0)
				{
					pos[p_index++] = --index;
					delete[d_index++] = keys[index];
					refresh_screen();
				}
				break;
			case ESC:
				esc_mode = 1;
				break;
			default:
				break;
			}
		}
	}
}
PRIVATE void back()
{
	if (p_index == 0 && d_index == 0)
		return;
	p_index--;
	index = p_index > 0 ? pos[p_index - 1] : 0;
	if (p_index > 0 && pos[p_index] < pos[p_index - 1])
	{

		keys[index - 1] = delete[--d_index];
	}
	refresh_screen();
}
PRIVATE void refresh_screen()
{
	clean_screen();
	char output[2] = {'\0', '\0'};
	for (int i = 0; i < index; i++)
	{
		if (keys[i] == '\n')
		{
			disp_pos = 160 * (disp_pos / 160 + 1);
			set_cursor(disp_pos);
		}
		else if (keys[i] == '\t')
		{
			int num = 4 - (disp_pos / 2 % 4);
			for (int i = 0; i < num; i++)
				disp_str(" ");
		}
		else
		{
			output[0] = keys[i];
			disp_str(output);
		}
	}
}
PRIVATE void str_match()
{
	int is_match = 0;
	char output[2] = {'\0', '\0'};
	for (int i = 0; i < index; i++)
	{
		int j = i;
		while (s_keys[j - i] == keys[j++])
			;
		if (j - i - s_index > 0)
		{
			for (int k = i; k < j - 1; k++)
			{
				output[0] = keys[k];
				disp_color_str(output, BLUE);
				i = k;
			}
		}
		else
		{
			if (keys[i] == '\n')
			{
				disp_pos = 160 * (disp_pos / 160 + 1);
				set_cursor(disp_pos);
			}
			else if (keys[i] == '\t')
			{
				int num = 4 - (disp_pos / 2 % 4);
				for (int i = 0; i < num; i++)
					disp_str(" ");
			}
			else
			{
				output[0] = keys[i];
				disp_str(output);
			}
		}
	}
}
PRIVATE void clean_screen()
{
	disp_pos = 0;
	for (int i = 0; i < 80 * 25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
}
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position / 2 >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position / 2 & 0xFF);
	enable_int();
}
