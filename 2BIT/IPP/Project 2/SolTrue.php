<?php

namespace IPP\Student;


class SolTrue extends SolBoolean
{
    private static ?self $instance = null;

    private function __construct(Interpreter $interpreter)
    {
        parent::__construct(true, $interpreter);
    }

    public static function getInstance(Interpreter $interpreter): self
    {
        return self::$instance ??= new self($interpreter);
    }
}
