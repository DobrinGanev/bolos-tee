
#******************************************************************************    
#*   BOLOS TEE Tools 
#*   (c) 2016 Ledger
#*   
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*   Unless required by applicable law or agreed to in writing, software
#*   distributed under the License is distributed on an "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*   See the License for the specific language governing permissions and
#*   limitations under the License.
#********************************************************************************

from __future__ import print_function
import sys
import json


class TooManyParametersException(Exception):
    pass

# This is not required if you've installed pycparser into
# your site-packages/ with setup.py
#sys.path.extend(['.', '..'])

from pycparser import c_parser, c_ast, parse_file

class BolosGenerator:
    def __init__(self, headerFile, mappingFile):
        self.mappingFile = mappingFile
        self.headerFile = headerFile
        self.bolosMapping = BolosMapping(mappingFile)            
        self.currentExportedFunction = None
        self.currentParameter = None
        self.exportedFunctionsList = []

    def setCurrentExportedFunction(self, currentExportedFunction):
        self.currentExportedFunction = currentExportedFunction

    def setCurrentParameter(self, currentParameter):
        self.currentParameter = currentParameter

    def getCurrentExportedFunction(self):
        return self.currentExportedFunction

    def getCurrentParameter(self):
        return self.currentParameter

    def getExportedFunctionsList(self):
        return self.exportedFunctionsList

    def getMapping(self):
        return self.bolosMapping

    def parse(self):
        ast = parse_file(self.headerFile, use_cpp=True,
                         cpp_args=r'-Iutils/fake_libc_include')

        #ast.show()

        v = DeclVisitor(self)
        v.visit(ast)

        self.bolosMapping.save()

    def generateClientStub(self, fileName):
        with open(fileName, 'w') as f:
            f.write('#include "syscall.h"\n')
            # Declare all functions
            for func in self.exportedFunctionsList:
                f.write('\t.globl\t %s\n' % (func.getCodeName()))
                f.write('\t.type\t %s,@function\n' % (func.getCodeName()))
            f.write('\t.text\n')
            # Write stub per functions
            for func in self.exportedFunctionsList:
                f.write('/*\n')
                f.write('* %s\n' % (func.getName()))
                regIndex = 0
                for parameter in func.getParameters():
                    f.write('* $r%d -- %s\n' % (regIndex, parameter.getName()))
                    regIndex += 1
                f.write('* Output:\n')
                f.write('* %s%s\n' % (func.getReturnParameter().getType(), "*" if (func.getReturnParameter().isPointer()) else ""))
                f.write('*/\n')
                f.write('%s:\n' % (func.getCodeName()))
                f.write('\tswi\t%d\n' % (func.getCallIndex()))
                f.write('\tret\n')
                f.write('.Lend%s:\n' % (func.getCodeName()))
                f.write('\t.size\t%s,.Lend%s-%s\n' % (func.getCodeName(), func.getCodeName(), func.getCodeName()))
                f.write('\n')

    def generateServerStub(self, fileName):
        with open(fileName, 'w') as f:
            f.write('#include <stdio.h>\n')
            f.write('#include <stdint.h>\n')
            f.write('#include <stdlib.h>\n')
            f.write('#include "machine.h"\n')
            f.write('\n')
            # Write stub per functions
            for func in self.exportedFunctionsList:
                f.write('/*\n')
                f.write('* %s\n' % (func.getName()))
                regIndex = 0
                for parameter in func.getParameters():
                    f.write('* $r%d -- %s %s%s\n' % (regIndex, parameter.getName(), parameter.getType(), "*" if (parameter.isPointer()) else ""))
                    regIndex += 1
                f.write('* Output:\n')
                f.write('* %s%s\n' % (func.getReturnParameter().getType(), "*" if (func.getReturnParameter().isPointer()) else ""))
                f.write('*/\n')
                #f.write('void moxie_%s(struct machine *mach) {\n' % (func.getCodeName()))
                #f.write('\t/* TODO */\n')
                #f.write('}\n')
                #f.write('\n')
                f.write('void moxie_%s(struct machine *mach);\n' % (func.getCodeName()))                
                f.write('\n')

    def generateServerDispatcher(self, fileName):
        with open(fileName, 'w') as f:
            f.write('#include <stdio.h>\n')
            f.write('#include <stdint.h>\n')
            f.write('#include <stdlib.h>\n')
            f.write('#include "machine.h"\n')
            f.write('\n')
            f.write('int moxie_swi_dispatcher(struct machine *mach, int swi) {\n')
            f.write('\tswitch(swi) {\n')
            # Write stub per functions
            for func in self.exportedFunctionsList:
                f.write('\t\tcase %d:\n' % (func.getCallIndex()))
                f.write('\t\t\tmoxie_%s(mach);\n' % (func.getCodeName()))
                f.write('\t\t\tbreak;\n')
            f.write('\t\tdefault:\n')
            f.write('\t\t\treturn 0;\n')
            f.write('\t}\n')
            f.write('\treturn 1;\n')
            f.write('}\n')

class BolosMapping:
    def __init__(self, mappingFileName):
        self.currentCallIndex = 0
        self.mappingFileName = mappingFileName
        try:
            with open(mappingFileName, 'r') as f:            
                self.mapping = json.load(f)
        except Exception:
            self.mapping = {}
        for i in self.mapping:
            if self.mapping[i] > self.currentCallIndex:
                self.currentCallIndex = self.mapping[i]
        self.currentCallIndex += 1

    def getCallIndex(self, name):
        if name in self.mapping:
            return self.mapping[name]
        self.mapping[name] = self.currentCallIndex
        self.currentCallIndex += 1
        return self.mapping[name]

    def save(self):
        with open(self.mappingFileName, 'w') as f:
            json.dump(self.mapping, f)

class Parameter:
    def __init__(self, name=None):
        self.name = name
        self.paramPointer = False
        self.paramConst = False

    def setType(self, paramType):
        self.paramType = paramType

    def setPointer(self, paramPointer):
        self.paramPointer = paramPointer

    def setConst(self, paramConst):
        self.paramConst = paramConst

    def getName(self):
        return self.name

    def getType(self):
        return self.paramType

    def isPointer(self):
        return self.paramPointer

    def isConst(self):
        return self.paramConst

class ExportedFunction:
    def __init__(self):
        self.parameters = []        
        self.callIndex = 0

    def setName(self, name):
        self.name = name

    def addParameter(self, parameter):
        self.parameters.append(parameter)

    def setReturnParameter(self, paramReturn):
        self.paramReturn = paramReturn

    def setCallIndex(self, callIndex):
        self.callIndex = callIndex    

    def getName(self):
        return self.name

    def getCodeName(self):
        return self.name

    def getParameters(self):
        return self.parameters

    def getReturnParameter(self):
        return self.paramReturn

    def getCallIndex(self):
        return self.callIndex

class BolosParameterVisitor(c_ast.NodeVisitor):
    def __init__(self, context):
        self.context = context

    def visit_PtrDecl(self, node):
        currentParameter = self.context.getCurrentParameter()
        #print("Pointer")        
        currentParameter.setPointer(True)
        for c_name, c in node.children():
            self.visit(c)

    def visit_IdentifierType(self, node):
        currentParameter = self.context.getCurrentParameter()
        currentParameter.setType(' '.join(node.names))
        #print("Type %s" % (node.names))

class BolosParameterListVisitor(c_ast.NodeVisitor):
    def __init__(self, context):
        self.context = context

    def visit_Decl(self, node):
        currentParameter = self.context.getCurrentParameter()
        #print("Param %s" % (node.name))
        currentParameter = Parameter(node.name)        
        for qual in node.quals:
            if qual == 'const':
                currentParameter.setConst(True)
        self.context.setCurrentParameter(currentParameter)                
        v = BolosParameterVisitor(self.context)
        v.visit(node)
        self.context.getCurrentExportedFunction().addParameter(currentParameter)

class BolosVisitor(c_ast.NodeVisitor):
    def __init__(self, context):
        self.returnPointer = False
        self.context = context

    def visit_ParamList(self, node):
        #print("paramList")
        v = BolosParameterListVisitor(self.context)
        v.visit(node)

    def visit_PtrDecl(self, node):
        #print("Pointer")        
        self.returnPointer = True
        for c_name, c in node.children():
            self.visit(c)        

    def visit_TypeDecl(self, node):
        currentExportedFunction = self.context.getCurrentExportedFunction()
        if len(currentExportedFunction.getParameters()) > 6:
            raise TooManyParametersException("Too many parameters for %s" % (node.declname))
        currentExportedFunction.setName(node.declname)
        currentExportedFunction.setCallIndex(self.context.getMapping().getCallIndex(node.declname))
        #print("typeDecl %s" % (node.declname))
        currentParameter = Parameter()
        currentParameter.setPointer(self.returnPointer)
        self.context.setCurrentParameter(currentParameter)
        v = BolosParameterVisitor(self.context)        
        v.visit(node)            
        currentExportedFunction.setReturnParameter(currentParameter)        

class DeclVisitor(c_ast.NodeVisitor):
    def __init__(self, context):
        self.context = context

    def visit_FuncDecl(self, node):
        #print("FuncDecl")
        self.context.setCurrentExportedFunction(ExportedFunction())
        v = BolosVisitor(self.context)
        v.visit(node)  
        self.context.getExportedFunctionsList().append(self.context.getCurrentExportedFunction())
        #print("FuncDecl end")      

if __name__ == "__main__":
    if len(sys.argv) > 2:
        filename  = sys.argv[1]
        mappingFile = sys.argv[2]

    generator = BolosGenerator(filename, mappingFile)    
    generator.parse()
    exportedFunctionsList = generator.getExportedFunctionsList()

    for func in exportedFunctionsList:
        print(func.getName())
        for parameter in func.getParameters():
            print("\t%s %s %s %s" % (parameter.getType(), parameter.getName(), parameter.isConst(), parameter.isPointer()))
        print("%s %s" % (func.getReturnParameter().getType(), func.getReturnParameter().isPointer()))
        print("-------------------------------------------------------")

    generator.generateClientStub('/tmp/bolos.S')
    generator.generateServerStub('/tmp/bolos_server.c')
    generator.generateServerDispatcher('/tmp/bolos_server_dispatch.c')

