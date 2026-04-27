export type Template = Array<unknown>;

export type EvaluationCallback = (
  type: "pre" | "post",
  valid: boolean,
  instruction: unknown[],
  evaluatePath: string,
  instanceLocation: string,
  annotation: unknown
) => void;

export type StandardOutputFormat = 'flag' | 'basic';

export interface StandardOutputErrorEntry {
  keywordLocation: string;
  absoluteKeywordLocation: string;
  instanceLocation: string;
  error: string;
}

export interface StandardOutputAnnotationEntry {
  keywordLocation: string;
  absoluteKeywordLocation: string;
  instanceLocation: string;
  annotation: unknown[];
}

export type StandardOutputFlagResult = { valid: boolean };

export type StandardOutputBasicResult =
  | { valid: true; annotations?: StandardOutputAnnotationEntry[] }
  | { valid: false; errors: StandardOutputErrorEntry[] };

export type StandardOutputResult =
  | StandardOutputFlagResult
  | StandardOutputBasicResult;

export declare class Blaze {
  static reviver(
    key: string,
    value: unknown,
    context: { source: string }
  ): unknown;
  constructor(template: Template);
  validate(instance: unknown, format: 'flag'): StandardOutputFlagResult;
  validate(instance: unknown, format: 'basic'): StandardOutputBasicResult;
  validate(instance: unknown, callback?: EvaluationCallback): boolean;
}

export declare function describe(
  valid: boolean,
  instruction: unknown[],
  evaluatePath: string,
  instanceLocation: string,
  instance: unknown,
  annotation: unknown
): string;
