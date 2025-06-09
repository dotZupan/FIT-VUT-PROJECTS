<?php

namespace IPP\Student;

use IPP\Core\ReturnCode;

class SolClass extends SolObject
{
    public function __construct(string $className, Interpreter $interpreter)
    {
        parent::__construct("Class", $interpreter, ["value" => $className]);
    }

    /**
     * Handles built-in selectors for class-level dispatch.
     * @param array<int, SolObject> $args
     */
    public function respondTo(string $selector, array $args, Interpreter $interpreter): mixed
    {
        return match ($selector) {
            "read" => $this->handleRead($args, $interpreter),
            "from:" => $this->handleFrom($args, $interpreter),
            "new" => $this->handleNew($args, $interpreter),
            default => parent::respondTo($selector, $args, $interpreter),
        };
    }

    /**
     * @param array<int, SolObject> $args
     */
    private function handleRead(array $args, Interpreter $interpreter): SolString
    {
        if (count($args) !== 0) {
            throw new \RuntimeException("read expects 0 arguments", ReturnCode::PARSE_ARITY_ERROR);
        }

        if ($this->getField("value") !== "String") {
            throw new \RuntimeException("read is only defined on String class", ReturnCode::INTERPRET_DNU_ERROR);
        }

        $input = $interpreter->getInput()->readString();
        return new SolString(rtrim($input), $interpreter);
    }

    /**
     * @param array<int, SolObject> $args
     */
    private function handleFrom(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 1) {
            throw new \RuntimeException("from: expects 1 argument", ReturnCode::PARSE_ARITY_ERROR);
        }

        $value = $args[0]->hasField("value") ? $args[0]->getField("value") : $args[0];
        $className = $this->getField("value");

        return match ($className) {
            "Integer" => $this->convertToInteger($value, $interpreter),
            "String" => new SolString((string)$value, $interpreter),
            "True" => $interpreter->getTrueInstance(),
            "False" => $interpreter->getFalseInstance(),
            "Nil" => $interpreter->getNilInstance(),
            default => new SolObject($className, $interpreter, ["value" => $value]),
        };
    }

    private function convertToInteger(mixed $value, Interpreter $interpreter): SolInteger
    {
        if (is_string($value) && !preg_match('/^[+-]?\d+$/', $value)) {
            throw new \RuntimeException("Cannot convert '$value' to Integer", ReturnCode::INTERPRET_VALUE_ERROR);
        }

        return new SolInteger((int)$value, $interpreter);
    }
    
    /**
     * @param array<int, SolObject> $args
     */
    private function handleNew(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 0) {
            throw new \RuntimeException("new expects 0 arguments", ReturnCode::PARSE_ARITY_ERROR);
        }

        $className = $this->getField("value");

        return match ($className) {
            "Nil" => $interpreter->getNilInstance(),
            "True" => $interpreter->getTrueInstance(),
            "False" => $interpreter->getFalseInstance(),
            "Object" => new SolObject("Object", $interpreter),
            "Integer" => new SolInteger(0, $interpreter),
            "String" => new SolString("", $interpreter),
            default => new SolObject($className, $interpreter),
        };
    }
}
