from dataclasses import dataclass, field
from typing import Callable


@dataclass
class Field:
    name: str
    min_length: int | None = None
    max_length: int | None = None


@dataclass
class SchemeSpec:
    name: str
    inputs: list[Field]
    outputs: list[str]
    run: Callable[[dict[str, bytes]], dict[str, bytes]]
    fixed_vectors: list[dict[str, str]]