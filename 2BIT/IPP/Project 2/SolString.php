<?php

namespace IPP\Student;

/**
 * Represents builtin String Object
 */
class SolString extends SolObject
{
    public function __construct(string $value, Interpreter $interpreter)
    {
        parent::__construct("String", $interpreter, ["value" => $value]);
    }

    /**
     * Dispatch selector to appropriate method
     */
    public function respondTo(string $selector, array $args, Interpreter $interpreter): mixed
    {
        return match ($selector) {
            "equalTo:" => $this->handleEqualTo($args, $interpreter),
            "asString" => $this,
            "asInteger" => $this->AsInteger($interpreter),
            "isString" => $interpreter->getTrueInstance(),
            "concatenateWith:" => $this->Concatenate($args, $interpreter),
            "startsWith:endsBefore:" => $this->StartsWithEndsBefore($args, $interpreter),
            "print" => $this->Print($interpreter),
            "read" => $this->Read($interpreter),
            default => parent::respondTo($selector, $args, $interpreter),
        };
    }

    private function Print(Interpreter $interpreter): SolString
    {
        $interpreter->getStdout()->writeString((string)$this->getField("value"));
        return $this;
    }


    private function Read(Interpreter $interpreter): SolString
    {
        $input = $interpreter->getInput()->readString();
        return new SolString(is_string($input) ? rtrim($input) : "", $interpreter);
    }
    
    /**
     * Compares this string with another.
     * 
     * @param array<int, SolObject> $args
     */
    private function handleEqualTo(array $args, Interpreter $interpreter): SolBoolean
    {
        if (count($args) !== 1) {
            return $interpreter->getFalseInstance();
        }

        $a = $this->getField("value");
        $b = $args[0];

        if ($b instanceof SolString) {
            return $a === $b->getField("value")
                ? $interpreter->getTrueInstance()
                : $interpreter->getFalseInstance();
        }

        if ($b->hasField("value")) {
            $inner = $b->getField("value");

            if ($inner instanceof SolObject) {
                return $this->respondTo("equalTo", [$inner], $interpreter);
            }

            return $a === $inner
                ? $interpreter->getTrueInstance()
                : $interpreter->getFalseInstance();
        }

        return $interpreter->getFalseInstance();
    }

    private function AsInteger(Interpreter $interpreter): SolObject
    {
        $val = $this->getField("value");
        return (is_string($val) && preg_match('/^[+-]?\d+$/', $val))
            ? new SolInteger((int)$val, $interpreter)
            : $interpreter->getNilInstance();
    }

    /**
     * Concatenates this string with another.
     * 
     * @param array<int, SolObject> $args
     */
    private function Concatenate(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 1) {
            return $interpreter->getNilInstance();
        }

        $a = $this->getField("value");
        $b = $this->resolveStringFromArg($args[0]);

        if (!is_string($a) || !is_string($b)) {
            return $interpreter->getNilInstance();
        }

        return new SolString($a . $b, $interpreter);
    }

    /**
     * Returns substring between given indexes.
     * 
     * @param array<int, SolObject> $args
     */
    private function StartsWithEndsBefore(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 2 || !($args[0] instanceof SolInteger) || !($args[1] instanceof SolInteger)) {
            return $interpreter->getNilInstance();
        }

        $start = $args[0]->getField("value");
        $end = $args[1]->getField("value");

        if (!is_int($start) || !is_int($end) || $start <= 0 || $end <= 0) {
            return $interpreter->getNilInstance();
        }

        if ($end <= $start) {
            return new SolString("", $interpreter);
        }

        return new SolString(substr($this->getField("value"), $start - 1, $end - $start), $interpreter);
    }

    /**
     * Attempts to extract a string from an argument
     */
    private function resolveStringFromArg(SolObject $arg): ?string
    {
        if ($arg instanceof SolString) {
            return $arg->getField("value");
        }

        if ($arg->hasField("value")) {
            $inner = $arg->getField("value");
            return match (true) {
                is_string($inner) => $inner,
                $inner instanceof SolString => $inner->getField("value"),
                default => null,
            };
        }

        return null;
    }
}
