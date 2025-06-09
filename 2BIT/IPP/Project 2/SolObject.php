<?php

namespace IPP\Student;

use DOMElement;
use IPP\Core\ReturnCode;

class SolObject
{
    public string $class_name;
    /** @var array<string, mixed> */
    public array $fields = [];
    protected Interpreter $interpreter;

    /**
     * @param array<string, mixed> $fields
     */
    public function __construct(string $class_name, Interpreter $interpreter, array $fields = [])
    {
        $this->class_name = $class_name;
        $this->interpreter = $interpreter;
        $this->fields = $fields;
    }

    /**
     * Gets value from field or null if doesn't exists
     */
    public function getField(string $name): mixed
    {
        return $this->fields[$name] ?? null;
    }

    /**
     * Writes or overwrites value in field
     */
    public function setField(string $name, mixed $val): void
    {
        $this->fields[$name] = $val;
    }

    /**
     * Checks if field has that value
     */
    public function hasField(string $name): bool
    {
        return array_key_exists($name, $this->fields);
    }

    /**
     * Process selector
     * First look in the classTable for the method 
     * @param array<int, SolObject> $args
     */
    public function respondTo(string $selector, array $args, Interpreter $interpreter): mixed
    {
        $methodData = $interpreter->findClassMethod($this->class_name, $selector);
        $method = $methodData['method'] ?? null;
        $definedIn = $methodData['definedIn'] ?? null;
    
        if ($this->isUserDefinedMethod($method)) {
            return $interpreter->invokeMethod($method, $this, $args);
        }
    
        if ($this->isBuiltinMethod($method)) {
            return $this->dispatchBuiltin($definedIn, $selector, $args, $interpreter);
        }
    
        if ($this->hasField($selector)) {
            return $this->getField($selector);
        }
    
        return $this->dispatchFallback($selector, $args, $interpreter);
    }
    
    /**  helpers   */
    private function isUserDefinedMethod(mixed $method): bool
    {
        return $method instanceof DOMElement;
    }
    
    private function isBuiltinMethod(mixed $method): bool
    {
        return $method === "builtin";
    }
 
    /**
     * Dispatches to builtin method based on class and selector.
     * 
     * @param array<int, SolObject> $args
     */
    private function dispatchBuiltin(?string $definedIn, string $selector, array $args, Interpreter $interpreter): mixed
    {
        return match ($definedIn) {
            "Integer" => (new SolInteger($this->getField("value"), $interpreter))->respondTo($selector, $args, $interpreter),
            "String" => (new SolString($this->getField("value"), $interpreter))->respondTo($selector, $args, $interpreter),
            "Block" => (new SolBlock($this->fields["__blockNode"], $this->fields["__env"], $interpreter))->respondTo($selector, $args, $interpreter),
            default => throw new \RuntimeException("Unsupported builtin dispatch from class $definedIn", ReturnCode::INTERPRET_DNU_ERROR),
        };
    }
    
    /**
     * Handles fallback selectors.
     * 
     * @param array<int, SolObject> $args
     */
    private function dispatchFallback(string $selector, array $args, Interpreter $interpreter): mixed
    {
        return match ($selector) {
            "equalTo:" => $this->equalTo($args),
            "identicalTo:" => $this->identicalTo($args),
            "asString" => new SolString("", $interpreter),
            "isNumber", "isString", "isBlock", "isNil" => $interpreter->getFalseInstance(),
            default => $this->tryResolveOrDelegate($selector, $args, $interpreter),
        };
    }
    

        
    /**
     * Fallback for unresolved
     * @param array<int, SolObject> $args
     */
    private function tryResolveOrDelegate(string $selector, array $args, Interpreter $interpreter): mixed
    {
        // Setter fallback
        if (str_ends_with($selector, ":") && count($args) === 1) {
            $field = substr($selector, 0, -1);
            $this->setField($field, $args[0]);
            return $this;
        }

        // Try to delegate on internal value if exists
        if ($this->hasField("value")) {
            $val = $this->getField("value");
            if ($val instanceof SolObject) {
                return $val->respondTo($selector, $args, $interpreter);
            }

            if (is_bool($val)) {
                return ($val ? $interpreter->getTrueInstance() : $interpreter->getFalseInstance())
                    ->respondTo($selector, $args, $interpreter);
            }
        }

        throw new \RuntimeException("Selector '$selector' not understood by {$this->class_name}", ReturnCode::INTERPRET_DNU_ERROR);
    }

    /**
     * Compare fields and if needed fallback to compering for identity
     * @param array<int, SolObject> $args
     */
    protected function equalTo(array $args): SolBoolean
    {
        if (count($args) !== 1) {
            throw new \RuntimeException("equalTo: expects 1 argument", ReturnCode::PARSE_ARITY_ERROR);
        }

        $other = $args[0];

        // Ak this obsahuje .value, skús porovnanie vnorených hodnôt
        if ($this->hasField("value") && $other->hasField("value")) {
            $a = $this->getField("value");
            $b = $other->getField("value");

            // Ak oba objekty majú hodnoty a sú rovnakého typu
            if ($a == $b) { // porovnávame hodnoty
                return $this->interpreter->getTrueInstance();
            }
        }

        foreach ($this->fields as $k => $v) {
            if (!$other->hasField($k)) return $this->interpreter->getFalseInstance();
            if ($v !== $other->getField($k)) return $this->interpreter->getFalseInstance();
        }

        return $this->identicalTo($args);
    }

    /**
     * Compare if 2 objects are identical
     * @param array<int, SolObject> $args
     */
    protected function identicalTo(array $args): SolBoolean


    {
        if (count($args) !== 1) {
            throw new \RuntimeException("identicalTo: expects 1 argument", ReturnCode::PARSE_ARITY_ERROR);
        }

        return ($args[0] === $this)
            ? $this->interpreter->getTrueInstance()
            : $this->interpreter->getFalseInstance();
    }

}