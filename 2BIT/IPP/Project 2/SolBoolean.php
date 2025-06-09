<?php

namespace IPP\Student;

use DOMElement;
use IPP\Core\ReturnCode;

/**
 * Abstract class representing boolean values true and false
 */
abstract class SolBoolean extends SolObject
{
    public bool $value;

    public function __construct(bool $value, Interpreter $interpreter)
    {
        parent::__construct($value ? "True" : "False", $interpreter, ["value" => $value]);
        $this->value = $value;
    }

    /**
     * Handles all built-in selectors for boolean values.
     * @param array<int, SolObject> $args
     */
    public function respondTo(string $selector, array $args, Interpreter $interpreter): mixed
    {
        return match ($selector) {
            'asString' => new SolString($this->value ? 'true' : 'false', $interpreter),
            'not' => $this->value ? $interpreter->getFalseInstance() : $interpreter->getTrueInstance(),
            'print' => $this->Print($interpreter),
            'and:' => $this->Logical($args, $interpreter, false),
            'or:' => $this->Logical($args, $interpreter, true),
            'ifTrue:ifFalse:' => $this->IfTrueIfFalse($args, $interpreter),
            default => parent::respondTo($selector, $args, $interpreter),
        };
    }

    private function Print(Interpreter $interpreter): SolBoolean
    {
        $interpreter->getStdout()->writeString($this->value ? "true" : "false");
        return $this;
    }

    /**
     * Generalized logic for both `and:` and `or:` using short-circuit evaluation.
     * @param array<int, SolObject> $args
     */
    private function Logical(array $args, Interpreter $interpreter, bool $isOr): SolBoolean
    {
        if (count($args) !== 1) {
            $name = $isOr ? "or:" : "and:";
            throw new \RuntimeException("$name expects 1 argument", ReturnCode::PARSE_ARITY_ERROR);
        }

        // Short-circuit
        if (($isOr && $this->value) || (!$isOr && !$this->value)) {
            return $isOr ? $interpreter->getTrueInstance() : $interpreter->getFalseInstance();
        }

        $result = $this->resolveBooleanArgument($args[0], $interpreter, $isOr ? "or:" : "and:");
        return $result->value ? $interpreter->getTrueInstance() : $interpreter->getFalseInstance();
    }

    /**
     * Helper that resolves argument to a boolean value for logical operations.
     */
    private function resolveBooleanArgument(mixed $arg, Interpreter $interpreter, string $context): SolBoolean
    {
        if ($arg instanceof SolBlock) {
            $result = $arg->invoke([]);
        } elseif ($arg instanceof SolBoolean) {
            return $arg;
        } elseif ($arg instanceof SolObject && $arg->hasField("value")) {
            $val = $arg->getField("value");
            if ($val instanceof SolBoolean) return $val;
            if (is_bool($val)) return $val ? $interpreter->getTrueInstance() : $interpreter->getFalseInstance();
            throw new \RuntimeException("$context: .value must be boolean or SolBoolean", ReturnCode::INTERPRET_TYPE_ERROR);
        } else {
            throw new \RuntimeException("$context: argument must be Block or Boolean", ReturnCode::INTERPRET_TYPE_ERROR);
        }

        if (!($result instanceof SolBoolean)) {
            throw new \RuntimeException("$context: result must be SolBoolean", ReturnCode::INTERPRET_TYPE_ERROR);
        }

        return $result;
    }

    /**
     * Handles conditional logic: executes one of two blocks based on this boolean's value.
     * @param array<int, SolObject> $args
     */
    private function IfTrueIfFalse(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 2) {
            throw new \RuntimeException("ifTrue:ifFalse: expects two arguments", ReturnCode::PARSE_ARITY_ERROR);
        }

        foreach ($args as $i => $arg) {
            if (!$this->isZeroArityBlock($arg)) {
                throw new \RuntimeException("Argument $i must be a callable block with arity 0", ReturnCode::INTERPRET_DNU_ERROR);
            }
        }

        return $this->value
            ? $args[0]->respondTo("value", [], $interpreter)
            : $args[1]->respondTo("value", [], $interpreter);
    }

    /**
     * Checks whether the given object can be called with selector 'value' and arity 0.
     */
    private function isZeroArityBlock(SolObject $obj): bool
    {
        $methodInfo = $obj->interpreter->findClassMethod($obj->class_name, "value");

        if (!$methodInfo || $methodInfo['method'] === "builtin") {
            return $obj instanceof SolBlock && $obj->getArity() === 0;
        }

        if ($methodInfo['method'] instanceof DOMElement) {
            $blockNode = $methodInfo['method']->getElementsByTagName('block')[0] ?? null;
            return $blockNode instanceof DOMElement && ((int)$blockNode->getAttribute('arity')) === 0;
        }

        return false;
    }
}
