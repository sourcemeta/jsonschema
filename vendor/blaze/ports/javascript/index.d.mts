export type Template = Array<unknown>;

export type EvaluationCallback = (
  type: "pre" | "post",
  valid: boolean,
  instruction: unknown[],
  evaluatePath: string,
  instanceLocation: string,
  annotation: unknown
) => void;

export declare class Blaze {
  constructor(template: Template);
  validate(instance: unknown, callback?: EvaluationCallback): boolean;
}
