/*
* (C) 2014 Jack Lloyd
*
* Distributed under the terms in doc/license.rst
*/

#include "apps.h"
#include <botan/libstate.h>

int rng_main(int argc, char* argv[])
   {
   if(argc == 1)
      {
      std::cout << "Usage: " << argv[0] << " [--raw-entropy] [n]\n";
      return 1;
      }

   try
      {
      const size_t amt = to_u32bit(argv[argc-1]);
      const bool raw = (argc == 3 && std::string(argv[1]) == "--raw-entropy");

      if(!raw)
         {
         AutoSeeded_RNG rng;
         std::cout << hex_encode(rng.random_vec(amt)) << "\n";
         }
      else
         {
         double total_collected = 0;

         Entropy_Accumulator accum(
            [amt,&total_collected](const byte in[], size_t in_len, double entropy_estimate)
            {
            std::cout << "Collected estimated "<< entropy_estimate << " bits in "
                      << hex_encode(in, in_len) << "\n";
            total_collected += entropy_estimate;
            return total_collected >= amt;
            });

         global_state().poll_available_sources(accum);
         }
      }
   catch(std::exception& e)
      {
      std::cout << "Error: " << e.what() << "\n";
      return 1;
      }

   return 0;
   }
