/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <iostream>
#include <vector>
#include <map>

void vector_test()
{
    // Create and initialize a vector with 3 integer values
    std::vector<int> numbers = {10, 20, 30};

    // Compute the sum
    int sum = 0;
    for (int value : numbers) {
        sum += value;
    }

    // Output the result
    std::cout << "Sum of vector entries: " << sum << std::endl;
}


void string_test()
{
    std::string s = "abc";
    s += "def";
    std::cout << "String: " << s << std::endl;
}

void map_test()
{
    std::map<int,int> m;

    m[1] = 2;

    std::cout << "Map m[1]: " << m[1] << std::endl;
}


int main() {

    vector_test();

    string_test();

    map_test();

    return 0;
}