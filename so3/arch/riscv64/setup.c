//
// Created by nico on 10.06.21.
//

/**
 * Low-level initialization before the main boostrap process.
 */
void setup_arch(void) {

        int i, tmp0, tmp1;

        /* Do something just to see if boot gets here */
        for (i = 0; i < 10; i++) {
        	tmp0 = i % 2;
        	tmp1 = tmp0 + 1;
        }
}
