<?php

namespace IPP\Student;

use IPP\Core\ReturnCode;
use IPP\Core\Exception;


/**
 * Represents builtin Object Integer
 * has arithmetical and logical operations
 */
class SolInteger extends SolObject
{
    public function __construct(int|string $value, Interpreter $interpreter)
    {
        parent::__construct("Integer", $interpreter, ["value" => (int)$value]);
    }

    /**
     * Process selector
     * @param array<int, SolObject> $args
     */
    public function respondTo(string $selector, array $args, Interpreter $interpreter): mixed
    {
        return match ($selector) {
            "equalTo:" => $this->handleEqualTo($args, $interpreter),
            "asString" => new SolString(strval($this->getField("value")), $interpreter),
            "asInteger" => $this,
            "greaterThan:" => $this->greaterThan($args, $interpreter),
            "plus:" => $this->arithmetic($args, fn($a, $b) => $a + $b, $interpreter),
            "minus:" => $this->arithmetic($args, fn($a, $b) => $a - $b, $interpreter),
            "multiplyBy:" => $this->arithmetic($args, fn($a, $b) => $a * $b, $interpreter),
            "divBy:" => $this->divBy($args, $interpreter),
            "isNumber" => $interpreter->getTrueInstance(),
            "timesRepeat:" => $this->handleTimesRepeat($args, $interpreter),
            default => parent::respondTo($selector, $args, $interpreter),
        };
    }


    /**
     * General processing of Arithmetical operations such as plus, minus, multiplyBy
     * @param array<int, SolObject> $args
     */
    private function arithmetic(array $args, callable $op, Interpreter $interpreter): SolInteger
    {
        if (count($args) !== 1) {
            throw new \RuntimeException("Arithmetic expects 1 argument", ReturnCode::INTERPRET_VALUE_ERROR);
        }

        $a = $this->getField("value");
        $b = null;

        // capture value of second operand
        if ($args[0] instanceof SolInteger) {
            $b = $args[0]->getField("value");
        } elseif ($args[0]->hasField("value")) {
            $val = $args[0]->getField("value");
            if (is_int($val)) {
                $b = $val;
            }
        }

        if (!is_int($b)) {
            throw new \RuntimeException("Arithmetic expects Integer", ReturnCode::INTERPRET_VALUE_ERROR);
        }

        return new SolInteger($op($a, $b), $interpreter);
    }

    /**
     * Division with check for division by 0
     * @param array<int, SolObject> $args
     */
    private function divBy(array $args, Interpreter $interpreter): SolInteger
    {
        
        if (count($args) !== 1 || !($args[0] instanceof SolInteger)) {
            throw new \RuntimeException("divBy: expects 1 Integer argument", ReturnCode::INTERPRET_VALUE_ERROR);
        }

       
        $b = $args[0]->getField("value");
       
        if ($b === 0) {
            throw new \RuntimeException("Division by zero", ReturnCode::INTERPRET_VALUE_ERROR);
        }

        return new SolInteger(intdiv($this->getField("value"), $b), $interpreter);
    }

    /**
     * Compare if two numbers are equal
     * @param array<int, SolObject> $args
     */
    private function handleEqualTo(array $args, Interpreter $interpreter): SolBoolean
    {
        if (count($args) !== 1) {
            return $interpreter->getFalseInstance();
        }
    
        $a = $this->getField("value");
    
        $b = $args[0];
        if ($b->hasField("value")) {
            $b_val = $b->getField("value");
    
            if (is_int($b_val)) {
                return $a === $b_val
                    ? $interpreter->getTrueInstance()
                    : $interpreter->getFalseInstance();
            }
    
            if ($b_val instanceof SolInteger) {
                return $a === $b_val->getField("value")
                    ? $interpreter->getTrueInstance()
                    : $interpreter->getFalseInstance();
            }
        }
    
        return $interpreter->getFalseInstance();
    }

    /**
     * Compare if first number is greater than the second
     * @param array<int, SolObject> $args
     */
    private function greaterThan(array $args, Interpreter $interpreter): SolBoolean
    {
        if (count($args) !== 1 || !($args[0] instanceof SolInteger)) {
            throw new \RuntimeException("greaterThan: expects Integer", ReturnCode::INTERPRET_TYPE_ERROR);
        }

        return $this->getField("value") > $args[0]->getField("value")
            ? $interpreter->getTrueInstance()
            : $interpreter->getFalseInstance();
    }


    /**
     * Starts block multiply times based on numeric value
     * @param array<int, SolObject> $args
     */
    private function handleTimesRepeat(array $args, Interpreter $interpreter): SolObject
    {
        if (count($args) !== 1 || !($args[0] instanceof SolBlock)) {
            throw new \RuntimeException("timesRepeat: expects block", ReturnCode::INTERPRET_TYPE_ERROR);
        }

        $count = $this->getField("value");
        if (!is_int($count) || $count <= 0) {
            return $interpreter->getNilInstance();
        }

        for ($i = 1; $i <= $count; $i++) {
            $args[0]->invoke([new SolInteger($i, $interpreter)]);
        }

        return $interpreter->getNilInstance();
    }
}