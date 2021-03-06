/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "python.hpp"

#include "python-utils.hpp"
#include "python-modules.hpp"

namespace python {

ScriptOutput PythonEngine::exec(
    const std::string& python_code,
    const Converter& vars,
    ScriptOutput::CaptureFlags flags)
{
    if ( !initialize() )
        return ScriptOutput{};

    ScriptOutput output;
    try {
        ScriptEnvironment env(output, flags);
        try {
            vars.convert(env.main_namespace());
            boost::python::exec(py_str(python_code), env.main_namespace());
            output.success = true;
        } catch (const boost::python::error_already_set&) {
            ErrorLog("py") << "Exception from python script";

            if (PyErr_ExceptionMatches(PyExc_SystemExit))
                Log("py", '!', 3) << "Called sys.exit";
            else
                PyErr_Print();
            // PyErr_Print will use the stderr as defined in ScriptEnvironment
            // therefore it needs to still be alive in this catch
        }
    } catch (const boost::python::error_already_set&) {
        // This external try/catch block is because ScriptEnvironment
        // executes some Python code which might throw exceptions
        ErrorLog("py") << "Exception on python script initialization";
    }

    return output;
}

ScriptOutput PythonEngine::exec_file(
    const std::string& file,
    const Converter& vars,
    ScriptOutput::CaptureFlags flags)
{
    if ( !initialize() )
        return ScriptOutput{};

    ScriptOutput output;
    try {
        ScriptEnvironment env(output, flags);
        try {
            vars.convert(env.main_namespace());
            boost::python::exec_file(py_str(file), env.main_namespace());
            output.success = true;
        } catch (const boost::python::error_already_set&) {
            ErrorLog("py") << "Exception from python script";

            if (PyErr_ExceptionMatches(PyExc_SystemExit))
                Log("py", '!', 3) << "Called sys.exit";
            else
                PyErr_Print();
            // PyErr_Print will use the stderr as defined in ScriptEnvironment
            // therefore it needs to still be alive in this catch
        }
    } catch (const boost::python::error_already_set&) {
        // This external try/catch block is because ScriptEnvironment
        // executes some Python code which might throw exceptions
        ErrorLog("py") << "Exception on python script initialization";
    }

    return output;
}

bool PythonEngine::initialize()
{
    if ( !Py_IsInitialized() )
    {
        /// \todo custom module factory
        PyImport_AppendInittab( "melanobot", &py_melanobot::initmelanobot );
        Py_Initialize();
    }

    if ( !Py_IsInitialized() )
    {
        ErrorLog("py") << "Could not initialize the interpreter";
        return false;
    }

    return true;
}

} // namespace python
