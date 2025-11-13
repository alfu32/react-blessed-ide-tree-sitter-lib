/**
 * TypeScript definitions for tree-sitter-tokenizer
 * Auto-generated for the Node.js addon.
 * Compatible with Node 18+ and N-API v7+.
 */

export interface Token {
    /** Horizontal position of token */
    x: number;

    /** Vertical position of token */
    y: number;

    /** The raw text content of the token */
    content: string;

    /** The full AST path (if available) */
    full_path: string;

    /** Length of the full_path string */
    full_path_length: number;
}

/**
 * Represents a persistent Tree-sitter parser manager instance.
 * Each instance owns its underlying C parser and state buffers.
 */
export class ParserManager {
    /**
     * Creates a new parser for the given language.
     * @param langId The Tree-sitter language identifier (e.g., "c", "python", "javascript").
     * @throws If the language is unsupported or parser creation fails.
     */
    constructor(langId: string);

    /**
     * Parse a source string and retrieve all visible tokens within a window.
     * @param source The code to tokenize.
     * @param x0 Optional horizontal start (default 0).
     * @param y0 Optional vertical start (default 0).
     * @param x1 Optional horizontal end (default 9999).
     * @param y1 Optional vertical end (default 9999).
     * @returns Array of tokens including coordinates and AST paths.
     */
    getAllVisibleTokens(
        source: string,
        x0?: number,
        y0?: number,
        x1?: number,
        y1?: number
    ): Token[];

    /**
     * Releases the underlying native ParserManager.
     * After calling this, the instance becomes invalid.
     */
    close(): void;
}

/**
 * Retrieves the list of available language grammars compiled into the library.
 * @returns Array of language identifiers, e.g., ["c", "cpp", "python", ...].
 */
export function getLanguages(): string[];
