@REM
@REM Copyright or © or Copr. IETR/INSA - Rennes (2017 - 2018) :
@REM
@REM Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
@REM Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
@REM
@REM Spider is a dataflow based runtime used to execute dynamic PiSDF
@REM applications. The Preesm tool may be used to design PiSDF applications.
@REM
@REM This software is governed by the CeCILL  license under French law and
@REM abiding by the rules of distribution of free software.  You can  use,
@REM modify and/ or redistribute the software under the terms of the CeCILL
@REM license as circulated by CEA, CNRS and INRIA at the following URL
@REM "http://www.cecill.info".
@REM
@REM As a counterpart to the access to the source code and  rights to copy,
@REM modify and redistribute granted by the license, users are provided only
@REM with a limited warranty  and the software's author,  the holder of the
@REM economic rights,  and the successive licensors  have only  limited
@REM liability.
@REM
@REM In this respect, the user's attention is drawn to the risks associated
@REM with loading,  using,  modifying and/or developing or reproducing the
@REM software by the user in light of its specific status of free software,
@REM that may mean  that it is complicated to manipulate,  and  that  also
@REM therefore means  that it is reserved for developers  and  experienced
@REM professionals having in-depth computer knowledge. Users are therefore
@REM encouraged to load and test the software's suitability as regards their
@REM requirements in conditions enabling the security of their systems and/or
@REM data to be ensured and,  more generally, to use and operate it in the
@REM same conditions as regards security.
@REM
@REM The fact that you are presently reading this means that you have had
@REM knowledge of the CeCILL license and that you accept its terms.
@REM


@echo off
cd ../..
rm -rf bin
mkdir bin
cd bin
cmake ..\ -G "Visual Studio 12" -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE
pause
