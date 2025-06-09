<?php
// author: Matúš Fignár - xfignam00

namespace IPP\Student;

use DOMElement;
use IPP\Core\AbstractInterpreter;
use IPP\Core\ReturnCode;



/**
 * Main interpreter class that executes SOL25 AST
 */
class Interpreter extends AbstractInterpreter
{
    protected SolBoolean $true;
    protected SolBoolean $false;
    protected SolNil $nil;

    /**
     * @var array<string, array{name: string, parent: string|null, methods: array<string, DOMElement|string>}>
     */
    protected array $classTable = [];

    protected function init(): void
    {
        parent::init();
        $this->true = SolTrue::getInstance($this);
        $this->false = SolFalse::getInstance($this);
        $this->nil = SolNil::getInstance($this);
        $this->registerBuiltins();
    }

    // Singletons and stream accessors
    public function getTrueInstance(): SolBoolean { return $this->true; }
    public function getFalseInstance(): SolBoolean { return $this->false; }
    public function getNilInstance(): SolObject { return $this->nil; }
    public function getInput(): \IPP\Core\Interface\InputReader { return $this->input; }
    public function getSource(): \IPP\Core\Interface\SourceReader { return $this->source; }
    public function getStdout(): \IPP\Core\Interface\OutputWriter { return $this->stdout; }

    /**
     * Main execution method: finds Main.run and invokes its block
     */
    public function execute(): int
    {
        try {
            $this->buildClassTable();

            $methodData = $this->findClassMethod("Main", "run");
            if (!$methodData || !($methodData['method'] instanceof DOMElement)) {
                throw new \RuntimeException("Main.run not found", ReturnCode::PARSE_MAIN_ERROR);
            }

            $blockNode = $this->extractMethodBlock($methodData['method']);
            $mainInstance = new SolObject("Main", $this);
            $mainBlock = new SolBlock($blockNode, ['self' => $mainInstance], $this);

            $mainBlock->invoke([]);
            return ReturnCode::OK;

        } catch (\Throwable $e) {
            $this->stderr->writeString("Error: " . $e->getMessage() . "\n");
            exit($e->getCode() ?: ReturnCode::INTERNAL_ERROR);
        }
    }

    /**
     * Extracts block from method node
     */
    private function extractMethodBlock(DOMElement $methodNode): DOMElement
    {
        foreach ($methodNode->childNodes as $child) {
            if ($child instanceof DOMElement && $child->tagName === 'block') {
                return $child;
            }
        }
        throw new \RuntimeException("Method missing block", ReturnCode::INVALID_SOURCE_STRUCTURE_ERROR);
    }

    /**
     * Invokes user-defined method (via block inside method node)
     *  @param array<int, SolObject> $args
     */
    public function invokeMethod(DOMElement $methodNode, SolObject $self, array $args ): SolObject
    {
        $blockNode = $this->extractMethodBlock($methodNode);
        $block = new SolBlock($blockNode, ['self' => $self], $this);

        if ($block->getArity() !== count($args)) {
            throw new \RuntimeException("Method expects {$block->getArity()} arguments", ReturnCode::PARSE_ARITY_ERROR);
        }

        return $block->invoke($args);
    }

    /**
     * Searches for a selector in class or its ancestors
     * @return array{method: DOMElement|string, definedIn: string}|null
     */
    public function findClassMethod(string $className, string $selector): ?array
    {
        while (isset($this->classTable[$className])) {
            $classInfo = $this->classTable[$className];
            if (isset($classInfo['methods'][$selector])) {
                return [
                    'method' => $classInfo['methods'][$selector],
                    'definedIn' => $className,
                ];
            }

            $className = $classInfo['parent'] ?? null;
        }
        return null;
    }

    /**
     * Builds class table from parsed AST
     */
    private function buildClassTable(): void
    {
        $document = $this->source->getDOMDocument();
        foreach ($document->getElementsByTagName('class') as $classNode) {
            $className = $classNode->getAttribute('name');
            $parent = $classNode->getAttribute('parent') ?: null;
            $methods = [];

            foreach ($classNode->getElementsByTagName('method') as $methodNode) {
                if ($methodNode->parentNode->isSameNode($classNode)) {
                    $selector = $methodNode->getAttribute('selector');
                    $methods[$selector] = $methodNode;
                }
            }

            $this->classTable[$className] = [
                'name' => $className,
                'parent' => $parent,
                'methods' => $methods,
            ];
        }
    }

    /**
     * Registers hardcoded builtin classes and methods
     */
    private function registerBuiltins(): void
    {
        $this->classTable["Object"] = ["name" => "Object", "parent" => null, "methods" => []];

        $this->classTable["Boolean"] = [
            "name" => "Boolean",
            "parent" => "Object",
            "methods" => [
                "not" => "builtin", "and:" => "builtin", "or:" => "builtin",
                "ifTrue:" => "builtin", "ifFalse:" => "builtin", "ifTrue:ifFalse:" => "builtin",
            ],
        ];

        $this->classTable["True"] = ["name" => "True", "parent" => "Boolean", "methods" => []];
        $this->classTable["False"] = ["name" => "False", "parent" => "Boolean", "methods" => []];

        $this->classTable["Nil"] = ["name" => "Nil", "parent" => "Object", "methods" => []];

        $this->classTable["Integer"] = [
            "name" => "Integer",
            "parent" => "Object",
            "methods" => [
                "plus:" => "builtin", "minus:" => "builtin", "multiplyBy:" => "builtin", "divBy:" => "builtin",
                "lessThan:" => "builtin", "greaterThan:" => "builtin",
                "asString" => "builtin", "asInteger" => "builtin",
                "timesRepeat:" => "builtin", "isInteger" => "builtin",
            ],
        ];

        $this->classTable["String"] = [
            "name" => "String",
            "parent" => "Object",
            "methods" => [
                "asString" => "builtin", "asInteger" => "builtin", "print" => "builtin", "read" => "builtin",
                "concatenateWith:" => "builtin", "isString" => "builtin",
            ],
        ];

        $this->classTable["Block"] = [
            "name" => "Block",
            "parent" => "Object",
            "methods" => [
                "value" => "builtin", "value:" => "builtin", "value:value:" => "builtin",
                "whileTrue:" => "builtin", "ifTrue:ifFalse:" => "builtin", "isBlock" => "builtin",
            ],
        ];

        $this->classTable["Class"] = [
            "name" => "Class",
            "parent" => "Object",
            "methods" => ["from:" => "builtin", "new" => "builtin"],
        ];
    }
}
