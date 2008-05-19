/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dab_insert_null_symbol.h>
#include <gr_io_signature.h>

/*
 * Create a new instance of dab_insert_null_symbol and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
dab_insert_null_symbol_sptr 
dab_make_insert_null_symbol (unsigned int fft_length, 
                       unsigned int cp_length, 
                       unsigned int symbols_per_frame,
                       unsigned int gap)
{
	return dab_insert_null_symbol_sptr (new dab_insert_null_symbol (fft_length, cp_length, symbols_per_frame, gap));
}

dab_insert_null_symbol::dab_insert_null_symbol (unsigned int fft_length, unsigned int cp_length, unsigned int symbols_per_frame, unsigned int gap) : 
	gr_block ("insert_null_symbol",
	           gr_make_io_signature2 (2, 2, sizeof(gr_complex), sizeof(char)),
	           gr_make_io_signature2 (2, 2, sizeof(gr_complex)*fft_length, sizeof(char))),
	d_state(STATE_NS), d_pos(0), d_fft_length(fft_length), d_cp_length(cp_length), d_symbols_per_frame(symbols_per_frame), d_sym_nr(0), d_gap(gap), d_gap_left(0)
{
  assert(gap<=cp_length);
}

void 
dab_insert_null_symbol::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
  int in_req  = noutput_items * (d_fft_length+d_cp_length);
  unsigned ninputs = ninput_items_required.size ();
  for (unsigned i = 0; i < ninputs; i++)
    ninput_items_required[i] = in_req;
}


int 
dab_insert_null_symbol::general_work (int noutput_items,
                        gr_vector_int &ninput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items)
{
	/* partially adapted from gr_insert_null_symbol.cc */
	const gr_complex *iptr = (const gr_complex *) input_items[0];
	const char *trigger = (const char *) input_items[1];
	
	gr_complex *optr = (gr_complex *) output_items[0];
	char *outsig = (char *) output_items[1];

	unsigned int n_in = ninput_items[0];
	unsigned int index = 0;
	unsigned int out = 0;

	switch (d_state) {
		case(STATE_NS):
			d_pos = 0;
			d_sym_nr = 0;
      d_gap_left = 0;
			while (index<n_in && !trigger[index])
				index++;
			if (trigger[index]) 
				d_state = STATE_CP;
			else
				break;
		case(STATE_CP):
      while (d_gap_left > 0 && index<n_in) { /* is there a gap left from the previous symbol? */
        index++;
        d_gap_left--;
      }
			while (index<n_in && d_pos < d_cp_length-d_gap) {
				index++;
				d_pos++;
			}
			if (d_pos == d_cp_length-d_gap)
				d_state = STATE_SYM;
			else
				break;
		case(STATE_SYM):
			if (index + d_fft_length <= n_in) {
				memcpy(&optr[out], &iptr[index], d_fft_length*sizeof(gr_complex));
				d_sym_nr++;
				index += d_fft_length;
				d_pos = 0;
				/* first symbol in frame? */
				if (d_sym_nr==1)
					outsig[out] = 1;
				else
					outsig[out] = 0;
				out++;
				/* last symbol in frame? */
				if (d_sym_nr == d_symbols_per_frame) {
					d_state = STATE_NS;
        } else {
					d_state = STATE_CP;
          d_gap_left = d_gap;
        }
			}
			break;
	}
	consume_each(index);
	return out;
}
