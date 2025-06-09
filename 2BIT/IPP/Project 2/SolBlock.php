<?php

namespace IPP\Student;

use DOMElement;
use IPP\Core\Exception\ParameterException;
use IPP\Core\ReturnCode;


/**
 * Represents block expression 
 */
class SolBlock extends SolObject
{
    private DOMElement $blockNode;

    /**
     *  Captures enviroment from the place where it was called
     *  @var array<string, SolObject> 
     * */
    private array $capturedEnv;


    /**
     * @param array<string, SolObject> $capturedEnv
     */
    public function __construct(DOMElement $blockNode, array $capturedEnv, Interpreter $interpreter)
    {
        parent::__construct("Block", $interpreter);
        $this->blockNode = $blockNode;
        $this->capturedEnv = $capturedEnv;
    }


    /**
     * Evaluates selector that was send on the block, take in account arity
     * @param array<int, SolObject> $args
     */
    public function respondTo(string $selector, array $args, Interpreter $interpreter): mixed
    {
        $arity = $this->getArity();
    
        return match ($selector) {
            'value' => $arity === 0 ? $this->invoke([]) : throw new \RuntimeException("Worng Arity", ReturnCode::INTERPRET_DNU_ERROR),
            'value:' => $arity === 1 ? $this->invoke($args) : throw new \RuntimeException("Worng Arity", ReturnCode::INTERPRET_DNU_ERROR),
            'value:value:' => $arity === 2 ? $this->invoke($args) : throw new \RuntimeException("Worng Arity", ReturnCode::INTERPRET_DNU_ERROR),
            'isBlock' => $interpreter->getTrueInstance(), 
            'whileTrue:' => $this->WhileTrue($args, $interpreter),
            'ifTrue:ifFalse:' => $this->IfTrueIfFalse($args, $interpreter),
            default => parent::respondTo($selector, $args, $interpreter),
        };
    }

    /**
     * Creates loacal enviroment and evaluates whole block
     * @param array<int, SolObject>|null $args
     * @return SolObject - Return value is value of last expression
     */
    public function invoke(mixed $args = null): SolObject
    {
        $env = $this->createLocalEnv($args ?? []);
        $statements = $this->collectOrderedStatements();
        $visitor = new Visitor($this->interpreter);
    
        $result = $this->interpreter->getNilInstance();
        foreach ($statements as $stmt) {
            $node = $stmt['node'];
            $result = $node->tagName === 'assign'
                ? $visitor->visitAssign($node, $env)
                : $visitor->visitExpr($node, $env);
        }
    
        return $result;
    }
    
    /**
     * @return array<int, array{order: int, node: DOMElement}>
     */
    private function collectOrderedStatements(): array
    {
        $statements = [];
    
        foreach ($this->blockNode->childNodes as $child) {
            if ($child instanceof DOMElement && in_array($child->tagName, ['assign', 'expr'])) {
                $order = (int)($child->getAttribute('order') ?: 0);
                $statements[] = ['order' => $order, 'node' => $child];
            }
        }
    
        usort($statements, fn($a, $b) => $a['order'] <=> $b['order']);
        return $statements;
    }

    /**
     * Evaluates parameters count that block expects
     * @return int
     */
    public function getArity(): int
    {
        $arity = $this->blockNode->getAttribute('arity');
        return $arity ? (int) $arity : 0;
    }


    /**
     * Creates local enviroment for block evaluation
     * Variables are named by params and paired with given args
     * @param array<int, SolObject> $args
     * @return array<string, SolObject>
     */
    private function createLocalEnv(array $args): array
    {
        $arity = $this->getArity();
    
        $paramList = [];
        foreach ($this->blockNode->getElementsByTagName('parameter') as $paramNode) {
            if (!$paramNode->hasAttribute('order') || !$paramNode->hasAttribute('name')) {
                continue;
            }
            $order = (int)$paramNode->getAttribute('order');
            $name = $paramNode->getAttribute('name');
            $paramList[$order] = $name;
        }
    
        // sort by correct order
        ksort($paramList);
        $paramList = array_values($paramList);
    
        if ($arity > count($paramList)) {
            throw new \RuntimeException("Argument count mismatch for block", ReturnCode::INTERPRET_DNU_ERROR);
        }
    
        $localEnv = ["self" => $this->capturedEnv["self"] ?? $this->interpreter->getNilInstance()];
        foreach ($paramList as $i => $name) {
            $localEnv[$name] = $args[$i] ?? $this->interpreter->getNilInstance();
        }
    
        return $localEnv;
    }
    
    /**
     * Implementation of builtin selector
     * Evaluates condition and repeatedly executes body while true
     * @param array<int, SolObject> $args
     */
    private function WhileTrue(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 1 || !($args[0] instanceof SolBlock)) {
            throw new ParameterException("whileTrue: expects a block argument");
        }

        $conditionBlock = $this->invoke([]); 
        $bodyBlock = $args[0]->invoke([]); 

        if (!$conditionBlock instanceof SolBoolean) {
            throw new \RuntimeException("Condition block must return SolBoolean", ReturnCode::INTERPRET_TYPE_ERROR);
        }
        
        /** @var SolBoolean $conditionBlock */
        while ($conditionBlock instanceof SolBoolean && $conditionBlock->value) {
            $bodyBlock = $args[0]->invoke([]);  // execution of the body
            $conditionBlock = $this->invoke([]);  // Re-evaluation of condition
        }

        return $interpreter->getNilInstance();  
    }


    /**
     * Evaluates current block as condition and based on its value calls one of two block
     * @param array<int, SolObject> $args
     */  
    private function IfTrueIfFalse(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 2) {
            throw new ParameterException("ifTrue:ifFalse: expects 2 arguments");
        }
        
        foreach ($args as $i => $arg) {
            try {
                $res = $arg->respondTo("value", [], $interpreter);
            } catch (\Throwable $e) {
                throw new \RuntimeException("Argument $i in ifTrue:ifFalse: is not invokable", ReturnCode::INTERPRET_DNU_ERROR);
            }
        }
        $conditionResult = $this->invoke([]);  // evaluation of condition

        if (!($conditionResult instanceof SolBoolean)) {
            throw new ParameterException("ifTrue:ifFalse: condition must return Boolean");
        }

        // based on condition call true or false block
        return $conditionResult->value
        ? $args[0]->respondTo("value", [], $interpreter)
        : $args[1]->respondTo("value", [], $interpreter);
    }
}