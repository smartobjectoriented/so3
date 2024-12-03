

Welcome to the Smart Object Oriented (SOO) Operating System (code name SO3)
***************************************************************************

For any information and discussions around SO3, please have a look at the [SO3 discussion forum](https://discourse.heig-vd.ch/c/so3).

Feel free to post any comments/suggestions/remarks about SO3. If you wish to participate to development, please simply ask us and we will manage separate branches of development. 

Furthermore, the full documentation is available [here](https://smartobjectoriented.github.io/so3)


We would like to extend our heartfelt thanks to our sponsors for their generous support in funding the development of the SO3 ecosystem, especially [HEIG-VD](http://www.heig-vd.ch) and the [Hasler Foundations](https://haslerstiftung.ch/en/welcome-to-the-hasler-foundation) 



Be careful with the requirements of various configs.

- avz_vt -> ./st must have virtualization on => use ./stv
- avz_pv -> ./st must have virtualization off => use ./st


To build the patch related to the CI:

- diff -Naur <source> <result> > so3_ci.patch
and put the ci/so3_ci.patch in ci/

To apply the patch, in the root:
- patch -p1 < ci/so3_ci.patch


