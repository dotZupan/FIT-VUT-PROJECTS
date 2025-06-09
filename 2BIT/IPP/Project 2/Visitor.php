<?php

namespace IPP\Student;

use DOMElement;
use IPP\Core\ReturnCode;

/**
 * Evaluates specific XML nodes from AST representation of the SOL25 language.
 */
class Visitor
{
    public function __construct(private Interpreter $interpreter)
    {
    }

    /**
     * Evaluates <assign> node by assigning value to variable.
     * @param array<array-key, SolObject> &$env
     */
    public function visitAssign(DOMElement $node, array &$env): SolObject
    {
        $varNode = $node->getElementsByTagName('var')[0];
        $exprNode = $node->getElementsByTagName('expr')[0];

        $varName = $varNode->getAttribute('name');
        $value = $this->visitExpr($exprNode, $env);

        $env[$varName] = $value;

        return $value;
    }

    /**
     * Dispatches evaluation based on inner node type.
     * @param array<array-key, SolObject> &$env
     */
    public function visitExpr(DOMElement $exprNode, array &$env): SolObject
    {
        foreach ($exprNode->childNodes as $child) {
            if ($child instanceof DOMElement) {
                return match ($child->tagName) {
                    'literal' => $this->visitLiteral($child),
                    'var' => $this->visitVar($child, $env),
                    'send' => $this->visitSend($child, $env),
                    'block' => $this->visitBlock($child, $env),
                    default => throw new \RuntimeException("Unknown expression: {$child->tagName}", ReturnCode::INTERNAL_ERROR),
                };
            }
        }

        throw new \RuntimeException("Empty <expr>", ReturnCode::INTERNAL_ERROR);
    }

    /**
     * Creates literal instance based on class name.
     */
    private function visitLiteral(DOMElement $node): mixed
    {
        $value = $node->getAttribute('value');
        $class = $node->getAttribute('class');

        if ($class === "String") {
            return new SolString($this->unescapeString($value), $this->interpreter);
        }

        return match ($class) {
            "Integer", "" => new SolInteger((int)$value, $this->interpreter),
            "True" => $this->interpreter->getTrueInstance(),
            "False" => $this->interpreter->getFalseInstance(),
            "Nil" => $this->interpreter->getNilInstance(),
            "class" => new SolClass($value, $this->interpreter),
            default => $value,
        };
    }

    /**
     * Looks up variable from local environment.
     * @param array<array-key, SolObject> &$env
     */
    public function visitVar(DOMElement $node, array &$env): SolObject
    {
        $name = $node->getAttribute('name');
        if (!array_key_exists($name, $env)) {
            throw new \RuntimeException("Undefined variable: $name", ReturnCode::PARSE_UNDEF_ERROR);
        }

        return $env[$name];
    }

    /**
     * Handles sending selector to evaluated receiver object.
     * @param array<array-key, SolObject> &$env
     */
    private function visitSend(DOMElement $node, array &$env): SolObject
    {
        $selector = $node->getAttribute('selector');
        $target = $this->extractTarget($node, $env);
        $args = $this->evaluateArguments($node, $env);

        return $target->respondTo($selector, $args, $this->interpreter);
    }

    /**
     *  Returns block instance with captured environment.
     *  @param array<array-key, SolObject> &$env
     */
    public function visitBlock(DOMElement $node, array &$env): SolObject
    {
        return new SolBlock($node, $env, $this->interpreter);
    }

    /**
     * Extracts target object (receiver) of a send node.
     * @param array<array-key, SolObject> &$env
     */
    private function extractTarget(DOMElement $node, array &$env): SolObject
    {
        foreach ($node->childNodes as $child) {
            if ($child instanceof DOMElement && $child->tagName === 'expr' && !$child->hasAttribute('order')) {
                return $this->visitExpr($child, $env);
            }
        }

        throw new \RuntimeException("Missing receiver for selector", ReturnCode::INVALID_SOURCE_STRUCTURE_ERROR);
    }

    /**
    * Evaluates and returns arguments of a send node, sorted by order.
    * @param array<array-key, SolObject> &$env
    * @return array<int, SolObject>
    */
    private function evaluateArguments(DOMElement $node, array &$env): array
    {
        $argNodes = [];
        foreach ($node->getElementsByTagName('arg') as $argNode) {
            if ($argNode->parentNode->isSameNode($node)) {
                $order = (int)($argNode->getAttribute('order') ?: 0);
                $argNodes[$order] = $argNode;
            }
        }

        ksort($argNodes);
        $args = [];

        foreach ($argNodes as $argNode) {
            $expr = $argNode->getElementsByTagName('expr')[0];
            $args[] = $this->visitExpr($expr, $env);
        }

        return $args;
    }

    /**
     * Process escaped characters in string literals.
     */
    private function unescapeString(string $value): string
    {
        return strtr($value, [
            '\\n' => "\n",
            '\\t' => "\t",
            '\\r' => "\r",
            '\\\\' => "\\",
            "\\'" => "'",
            '\\"' => '"',
        ]);
    }
}
