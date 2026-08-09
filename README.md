Unify
=====

Run Unity Unit Tests on RISC OS.

Introduction
------------

Unify is tool for running unit tests created in [Unity](https://www.throwtheswitch.org/unity) from https://www.throwtheswitch.org on RISC OS. The intended target are test suites with the structure used in the C applications found on [SFTools build environment](https://github.com/steve-fryatt), although it may be possible to extend it to use other project structures as well.


Building
--------

Unify consists of a collection of C and un-tokenised BASIC, which must be assembled using the [SFTools build environment](https://github.com/steve-fryatt). It will be necessary to have suitable Linux system with a working installation of the [GCCSDK](http://www.riscos.info/index.php/GCCSDK) to be able to make use of this.

With a suitable build environment set up, making Unify is a matter of running

	make

from the root folder of the project. This will build everything from source, and assemble a working !Unify application and its associated files within the build folder. If you have access to this folder from RISC OS (either via HostFS, LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make clean

To make a release version and package it into Zip files for distribution, use

	make release

This will clean the project and re-build it all, then create a distribution archive (no source), source archive and RiscPkg package in the folder within which the project folder is located. By default the output of `git describe` is used to version the build, but a specific version can be applied by setting the `VERSION` variable -- for example

	make release VERSION=1.23

Unit tests can be built using

	make test


Unit tests
----------

Unit tests for some parts of Unify are implemented using [Unity](https://www.throwtheswitch.org/unity) from https://www.throwtheswitch.org, with the tests defined within the `test/tests/` folder. When `make test` is run, a collection of absolute files are created in `test/absolute/` and these can be run on a RISC OS system using the `RunTests` TaskObey file in the same folder.

Test coverage is nowhere near complete! The focus is mainly on the back-end code away from the user interface and, due to the relatively recent addition of testing to the codebase, on parts of the code which have recently seen active development.


Licence
-------

Unify is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.