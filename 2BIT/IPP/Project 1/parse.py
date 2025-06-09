#!/usr/bin/env python3
# Parse.py

import sys
import re
import argparse
import xml.etree.ElementTree as ET
from lark import Lark, Token, Tree, Transformer, UnexpectedEOF, UnexpectedToken, UnexpectedCharacters
from xml.dom import minidom


# Grammar for Lark based on one from specification
grammar = r"""

    
    %import common.SIGNED_INT
    %import common.WS
    CLASS: "class"
    CID: /[A-Z][A-Za-z0-9_]*/

    
    COMMENT: /"([^"\\]*(\\.[^"\\]*)*)"/

    ?program: class_def*

    class_def: CLASS CID ":" CID "{" method* "}"
    method: selector block |  

    selector: ID | IDCOLON selector_tail
    selector_tail: (IDCOLON)*  

    block: "[" block_par "|" block_stat "]"
    block_par: (COLONID)*
    block_stat: (ID ":=" expr ".")* 

    expr: expr_base (expr_tail)?

    expr_tail: ID | expr_sel
    expr_sel: (IDCOLON expr_base)+
    expr_base: block | INT | STR | ID | CID | "(" expr ")"

    INT: /[+-]?\d+/
    STR: /'(?:[^\n\\']|\\(?:'|n|\\))*'/

    ID: /[a-z_][a-zA-Z0-9_]*/  
    IDCOLON: /[a-z_][a-zA-Z0-9_]*:/
    COLONID: /:[a-z_][a-zA-Z0-9_]*/


    %ignore WS
    %ignore COMMENT


"""

# Handles program arguments
# Parses command-line arguments and handles errors related to input arguments
# Returns the source code to be parsed (either from a file or standard input)
def parse_arguments():
    parser = argparse.ArgumentParser(description="SOL25 parser", add_help=False)
    parser.add_argument('--help', '-h', action='store_true', help='shows this help message')
    parser.add_argument('--source', type=str, help='input file with SOL25 source code (when not included, reading from stdin)' )
    
    args, unknown = parser.parse_known_args()

    if unknown:
        sys.stderr.write("Error 10: wrong argumen/s try to use --help or -h\n")
        sys.exit(10)

    elif args.help:
        if len(sys.argv) > 2:
            sys.stderr.write("Error 10: --help or -h cannot be combined with other arguments\n")
            sys.exit(10)
        else:
            parser.print_help()
            sys.exit(0)

    elif args.source:
        try:
            with open(args.source, 'r', encoding='utf-8') as file:
                return file.read()
        except FileNotFoundError:
            sys.stderr.write(f"Error 11: file {args.source} not found\n")
            sys.exit(11)

    else:
        return sys.stdin.read()


# Parses SOL25 code using the defined grammar
# Returns the parsed syntax tree or exits with an error code if parsing fails
def parse_code(parser, source):
    try:
        tree = parser.parse(source)
        
    except (UnexpectedEOF) as e:
        sys.stderr.write(f"{e}  22 Syntax error\n")
        sys.exit(22)

    except (UnexpectedToken) as e:
        sys.stderr.write(f"{e} 22 Syntax error\n")
        sys.exit(22)

    except UnexpectedCharacters as e:
        sys.stderr.write(f"{e} 21 Lexical error\n")
        sys.exit(21)

    except Exception as e:
        sys.stderr.write(f"{e} 99 Internal error\n")
        sys.exit(99)

    return tree



# Transforms the syntax tree into a structured data representation
# Extracts classes, methods, and other constructs from the parsed syntax tree    
class SOL25Transformer(Transformer):
    def __init__(self):
        self.classes = [] 

    def program(self, values):
        return values  
    
    def visit_token(self, token):
        return token.value

    # Transforms class definition from AST into a dictionary representation
    def class_def(self, values):                
        Class, name, parent, *methods = values  
        class_dict = {"name": name, "parent": parent, "methods": methods}
        self.classes.append(class_dict)
        return class_dict

    # Transforms method definitions
    def method(self, values):
        if not values:
            return None  
        
        selector, block = values
        return {"selector": selector, "block": block}


    def selector_tail(self, values):
        return values

    
    def selector(self, values):
        selector_parts = []
        if len(values) < 2:
            if "value" in values[0]:
                selector_parts.append(values[0]["value"])
            else:
                selector_parts.append(values[0]["selector"])

        else:
            for value in values:
                if isinstance(value, list):
                    for val in value:
                        selector_parts.append(val["selector"])
                else:
                    selector_parts.append(value["selector"])
            
        return "".join(selector_parts)

    def extract_selector_parts(self, tree):
        parts = []
        for child in tree.children:
            if isinstance(child, str):  
                parts.append(child)
            elif isinstance(child, Token): 
                parts.append(str(child))
            elif isinstance(child, Tree):  
                parts.extend(self.extract_selector_parts(child))
        return parts
    
    def block(self, values):   
        params, statements = values
        return {"params": params,"arity":len(params), "block_stat": statements}

    def block_par(self, values):
        return values  


    # This function is more robust than it should be but this assigment is not worth that much to fix that
    def block_stat(self, values):
        statements = []

        i = 0
        while i < len(values):
            var = values[i]  # The variable being assigned
            if (i + 1) < len(values):  
                expr = values[i + 1]  # The expression to be assigned

                if "type" in expr:
                    if expr["type"] == "literal":
                        if expr["class_name"] == "Integer":
                            statements.append({"variable": var, "expression": expr})
                        
                        elif expr["class_name"] == "String":
                            if expr["value"] == "nil":
                                expr["class_name"] = "Nil"
                                statements.append({"variable": var, "expression": expr})

                            elif expr["value"] == "true":
                                expr["class_name"] = "True"
                                statements.append({"variable": var, "expression": expr})
                            
                            elif expr["value"] == "False":
                                expr["class_name"] = "False"
                                statements.append({"variable": var, "expression": expr})
                            
                            else:
                                statements.append({"variable": var, "expression": expr})   

                    else:
                        statements.append({"variable": var, "expression": expr})

                elif "params" in expr:
                    expr_el = {"type": "block", "arity" : expr["arity"], "params" : expr["params"], "block_stat" : expr["block_stat"] }
                    statements.append({"variable": var, "expression" : expr_el})
               
                else:
                    statements.append({"variable" : var, "expression" : send(expr)})    


                i += 2  # Move to next assignment
            else:
                i += 1
        return statements

       

    def expr_sel(self, values):
        buffer = []
        selector = ""
        
        i = 0
        while i < len(values):
            if(i % 2 == 0):
                selector += values[i]["selector"]
                if "args" in values[i]:
                    for args in values[i]["args"]:
                        buffer.append(args)

            else:
                if isinstance(values[i], list):
                    for value in values[i]:
                        buffer.append(value)
                else:
                    buffer.append(values[i]) #args
            i += 1

        return {"selector" : selector, "args" : buffer}

    def expr(self, values):
        if(len(values) > 1):
            ret = send(values)
            return ret
        
        return values[0]

    def expr_base(self, values):
        return values[0]

    def expr_tail(self, value):
        ret = value[0]
        ret["type"] = "tail"
        return  ret

    

    def INT(self, value):
        return {"type": "literal", "class_name":"Integer", "value": int(value)}

    def STR(self, value):
        return {"type": "literal", "class_name": "String", "value":value[1:-1]}  

    def CID(self, value):
        return value  

    def ID(self, value):
        keywords = ["nil", "true", "false"]
        if value in keywords:
            return {"type": "literal", "class_name": "String", "value":value}
        
        elif value in ["self", "super"]:
            return {"type" : "var", "pseudo" : "true", "value" : value}
        
        elif value in ["new", "read", "asString", "isNumber", "isString", "isBlock", "isNil", "asInteger", "print", "not"]:
            return {"type" : "tail", "selector" : value}
        
        return {"type": "var", "value": value} 

    def IDCOLON(self, value):
            return {"type" : "tail", "selector" : value}


def send(expr):
    if "type" in expr[0]:
        reciever = expr[0]
        
    else:
        reciever = {"type" : "literal", "class" : "yes", "class_name" : expr[0]}
        
    expr_sel = expr[1]
    if "selector" in expr_sel:
        buffer = expr_sel["selector"]

    elif "value" in expr_sel:
        buffer = expr_sel["value"]
    else:
        sys.stderr.write("Error 35: Unexpected expression\n")
        sys.exit(35)
        

    if "args" in expr_sel:
        if "pseudo" in expr_sel["args"][0]:
            expr_sel["args"] = [send(expr_sel["args"])]
        expr_el = {"type" : "send", "selector" : buffer, "accepter" : reciever, "args" : expr_sel["args"]}
        
    else:
        expr_el = {"type" : "send", "selector" : buffer, "accepter" : reciever}
        
    return expr_el  



# Performs semantic analysis on the structure created by Transformer
# Checks for undefined classes, method redefinitions, and other semantic errors
#
# At initialization creates classes dictionary that contains all classes with its  
# parents classes and methods dictionary that contains all classes with its methods
# 
class semantic_check():
    def __init__(self):
        self.classes = {"Object" : "", "True" : "Object", "False" : "Object", "Integer" : "Object", "String" : "Object", "Block" : "Object"} #key is builtin class value is parent
        self.methods = {"Object": ["identicalTo:", "equalTo", "asString", "isNumber", "isString", "isBlock", "isNil", "from:", "new"],       #key is builtin class value is list with builtin mehods
                        "Integer": ["equalTo:", "graterThan:", "plus:", "minus:", "multiplyBy:", "divBy:", "asString", "asInteger", "timesRepeat:"],
                        "String" : ["read", "print", "equalTo", "asString", "asInteger", "concatenateWith:", "startsWith:endsBefore:"],
                        "Block" : ["whileTrue"],
                        "True" : ["not", "and:", "or:", "ifTrue:ifFalse:"],
                        "False" : ["not", "and:", "or:", "ifTrue:ifFalse:"]
                        }
        
        self.unresolved = [] 
        self.main_found = False  
        self.run_found = False  
        self.reserved_words = {"self", "super", "nil", "true", "false", "class"}
        


    def analyse(self, node):
        
        if isinstance(node, dict):
            node = [node]
        self.first_run(node)
        
        for class_ in node:
            if class_["parent"] not in self.classes:
                sys.stderr.write("ERROR undefined class\n")
                sys.exit(32)

            for method in class_["methods"]:
                self.method(method)
        return

    # Go through the data get and save every class with its parent into classes dictionary
    # add the class with its methods into methods dictionary for future use
    # and lastly check for the circular inheritance
    def first_run(self, node):
        for cl in node:
            name = cl["name"]
            if name in self.classes:
                sys.stderr.write("ERROR 35 redefinition of class\n")
                sys.exit(35)

            self.classes[name] = cl["parent"]
            self.methods[name] = []

            if name == "Main":
                self.main_found = True

            for method in cl["methods"]:
                if method["selector"] in self.methods[name]:
                    sys.stderr.write("ERROR 35: redefinition of method")
                    sys.exit(35)

                self.methods[name].append(method["selector"])

                if method["selector"] == "run" and name == "Main":
                    self.run_found = True

        if self.main_found == False or self.run_found == False:
            sys.stderr.write("ERROR 31 Class Main or methon run in Main is missing\n")
            sys.exit(31)

        
        self.check_circular_inheritance(self.classes)
        return
            
    def check_circular_inheritance(self, inheritance_graph):
        visited = set()
        stack = set()  # Helper stack for detection of cycles

        def dfs(class_name):
            if class_name in stack:  # Cyclical refrention
                sys.stderr.write("ERROR 35 Circular inheritance detected\n")
                sys.exit(35)

            if class_name in visited:  # We controlled it already
                return

            visited.add(class_name)
            stack.add(class_name)

            parent = inheritance_graph.get(class_name)
            if parent:  
                dfs(parent)

            stack.remove(class_name)

        # Check the class in the graph
        for cls in inheritance_graph:
            if cls not in visited:
                dfs(cls)
        return       


    def method(self, node):
        block = node["block"]

        if node["selector"] in self.reserved_words:
            sys.stderr.write("ERROR 22 Syntax errorr\n")
            sys.exit(22)

        selct_count = node["selector"].count(':')
        if selct_count != block["arity"]:
            sys.stderr.write("ERROR 33 wrong arity\n")
            sys.exit(33)

        self.block(block["block_stat"], block["params"])
        return 
        

    def block(self, node, params):
        if len(params) != len(set(params)):
            sys.stderr.write("ERROR 35 duplicit parameters\n")
            sys.exit(35)

        for i in range(len(params)):
            params[i] = params[i][1:]
            if params[i] in self.reserved_words:
                sys.stderr.write("ERROR 22 Syntax errorr\n")
                sys.exit(22)

        defined_vars = []
        for stat in node:
            var = stat["variable"]
            expr = stat["expression"]
            if (var["value"] in self.reserved_words):
                sys.stderr.write("ERROR 22 Syntax errorr\n")
                sys.exit(22)

            elif (var["value"] in params):
                sys.stderr.write("ERROR 34 collision var\n")
                sys.exit(34)


            elif (expr["type"] == "var") and expr["value"] not in defined_vars and (expr["value"] not in params):
                sys.stderr.write("ERROR 32 Undedefined var\n")
                sys.exit(32)

            elif expr["type"] == "send":
                self.send(expr, params, defined_vars)

            elif expr["type"] == "block":
                self.block(expr["block_stat"], expr["params"])

            defined_vars.append(var["value"])   
        return
    

    def send(self, node, params, defined):
        select = node["selector"]
        if select in self.reserved_words:
            sys.stderr.write("ERROR 22 Syntax errorr\n")
            sys.exit(22)

        if "class" in node["accepter"].keys():
            class_name = node["accepter"]["class_name"]
            if class_name not in self.classes:
                sys.stderr.write("ERROR 32 Undedefined var or class in expr\n")
                sys.exit(32)
            
            if self.check_method(class_name, select) == False:
                sys.stderr.write("ERROR 32 Undedefined var or class in expr\n")
                sys.exit(32)

        if "args" in node:
            for arg in node["args"]:
                if "type" in arg:
                    if arg["type"] == "var" and arg["value"] not in defined and arg["value"] not in params:
                        sys.stderr.write("ERROR 32 Undedefined var or class\n")
                        sys.exit(32)
                    
                    if arg["type"] == "send":
                        self.send(arg, params, defined)

                elif "block_stat" in arg:
                    self.block(arg["block_stat"], arg["params"])
        return


    def check_method(self, class_name, select):
        while True:
            if select in self.methods[class_name]:
                return True
            else:
                if class_name != "Object":
                    class_name = self.classes[class_name]
                else:
                    return False

# Generates XML output from the AST
# Converts the structured representation of the SOL25 program into XML format
def generate_xml(ast_root, comment):
    """Generates XML from the AST with proper formatting."""
    if comment:
        safe_comment = comment.replace("\n", "&#10;")
        root = ET.Element("program", language="SOL25", description=safe_comment)

    else:
        root = ET.Element("program", language="SOL25")

    if isinstance(ast_root, dict):
        ast_root = [ast_root]

    for class_node in ast_root:
        class_element = process_class(class_node)
        root.append(class_element)

    return root

# Processes individual parts of the AST into XML format
def process_class(node):
    class_el = ET.Element("class", name=node["name"], parent=node["parent"])
    for method in node["methods"]:
        method_el = process_method(method)
        class_el.append(method_el)
    return class_el

def process_method(node):
    method_el = ET.Element("method", selector=node["selector"])
    block = process_block(node["block"])
    method_el.append(block)

    return method_el

def process_block(node):
    block_el = ET.Element("block", arity=str(node["arity"]))
    
    for order in range(node["arity"]):
        params = ET.Element("parameter", order=str(order+1), name=node["params"][order])
        block_el.append(params)

    order = 0
    for block_st in node["block_stat"]:
        block_stat = process_block_stat(block_st, order)
        block_el.append(block_stat)
        order += 1
    return block_el

def process_block_stat(node, order):
    block_stat = ET.Element("assign", order=str(order+1))
    var = ET.Element("var", name=node["variable"]["value"])
    block_stat.append(var)

    expr = process_expr(node["expression"])
    block_stat.append(expr)
    return block_stat
    
def process_expr(node):
    expr = ET.Element("expr")

    if "type" not in node:
        expr.append(process_block(node))

    elif node["type"] == "literal":
        if "value" not in node.keys() :
            expr_base = ET.Element("literal", {"class" : "class", "value" : node["class_name"]})
            expr.append(expr_base)

        else:
            if node["value"] in ["true", "false", "nil"]:
                class_name = node["value"].capitalize()
            else:
                class_name = node["class_name"]

            expr_base = ET.Element("literal", {"class": class_name, "value": str(node["value"])})
            expr.append(expr_base)

    elif node["type"] == "var":
        expr_base = ET.Element("var", name=node["value"])
        expr.append(expr_base)
    
    elif node["type"] == "block":
        expr_base = process_block(node)
        expr.append(expr_base)

    elif node["type"] == "send":
        expr_base = ET.Element("send", selector=node["selector"])
        expr_sel = process_expr(node["accepter"])
        expr_base.append(expr_sel)
        
        if "args" in node:
            i = 1
            for args in node["args"]:
                arg = ET.Element("arg", order=str(i))
                expr_arg = process_expr(args)
                arg.append(expr_arg)
                expr_base.append(arg)
                i += 1
        expr.append(expr_base)
    
    return expr


# Formats and prints the XML output
def write_pretty_xml(xml_root):
    """Formats XML to ensure correct declaration and structure."""
    raw_xml = ET.tostring(xml_root, encoding="unicode")

    parsed_xml = minidom.parseString(raw_xml)
    formatted_xml = parsed_xml.toprettyxml(indent="    ")

    formatted_xml = formatted_xml.replace('<?xml version="1.0" ?>', '<?xml version="1.0" encoding="UTF-8"?>', 1)
    
    #formatted_xml = formatted_xml.replace('"/>', '" />')
    formatted_xml = formatted_xml.replace('&amp;#10;', '&#10;')
    
    #formatted_xml = "\n".join(line for line in formatted_xml.split("\n") if line.strip())

    print(formatted_xml)

# Extracts the first comment from the SOL25 code
# Used to include a description of the program in the XML output
def extract_first_comment(code):
    comment = re.compile(r'"([^"]+)"')
    match = comment.search(code)
    if match:
        return match.group(1)
    return None



def main():
    source = parse_arguments()
    comment = extract_first_comment(source)

    parser = Lark(grammar, start='program', parser='lalr', lexer="basic")
    tree = parse_code(parser, source)

    transformer = SOL25Transformer()
    data = transformer.transform(tree)
    analyzer = semantic_check()

    analyzer.analyse(data)
    xml_root = generate_xml(data, comment)
    write_pretty_xml(xml_root)

    return 0



if __name__ == '__main__':
    main()

