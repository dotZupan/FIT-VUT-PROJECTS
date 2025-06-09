<?php

namespace IPP\Student;


class SolFalse extends SolBoolean
{
    private static ?self $instance = null;

    private function __construct(Interpreter $interpreter)
    {
        parent::__construct(false, $interpreter);
    }

    public static function getInstance(Interpreter $interpreter): self
    {
        return self::$instance ??= new self($interpreter);
    }
}