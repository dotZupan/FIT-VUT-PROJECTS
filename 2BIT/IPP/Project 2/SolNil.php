<?php

namespace IPP\Student;

use IPP\Student\SolObject;
use IPP\Core\ReturnCode;


class SolNil extends SolObject
{

    protected Interpreter $interpreter;
    private static ?self $instance = null;

    private function __construct(Interpreter $interpreter)
    {
        parent::__construct("Nil", $interpreter, ['value' => null]);
    }

    public static function getInstance(Interpreter $interpreter): self
    {
        if (self::$instance === null) {
            self::$instance = new self($interpreter); // Only create one instance
        }
        return self::$instance;
    }

    /**
     * Process selectors
     * @param array<int, SolObject> $arr
     */
    public function respondTo(string $selector, array $arr, Interpreter $interpreter): mixed
    {
        return match ($selector)
        {
            "isNil" => $interpreter->getTrueInstance(),
            "equalTo:" => $this->equalTo($arr),
            "asString" => new SolString("nil", $interpreter),
            default => parent::respondTo($selector, $arr, $interpreter)
        };
    }

    /**
     * Compare if both operands are Instance of Nil
     * @param array<int, SolObject> $args
     */
    protected function equalTo(array $args): SolBoolean
    {
        if (count($args) !== 1) {
            throw new \RuntimeException("equalTo: expects 1 argument", ReturnCode::PARSE_ARITY_ERROR);
        }

        $arg = $args[0];
        if ($arg instanceof SolBlock) {
            $arg = $arg->invoke($args);
        }

        return $arg instanceof SolNil
            ? $this->interpreter->getTrueInstance()
            : $this->interpreter->getFalseInstance();
    }

    
}
