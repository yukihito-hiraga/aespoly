from dataclasses import dataclass
from typing import Callable
from typing import Protocol

@dataclass
class Field:
    name: str
    min_length: int | None = None
    max_length: int | None = None
    unit:int = 1


@dataclass
class SchemeSpec:
    name: str
    inputs: list[Field]
    outputs: list[str]
    fixed_vectors: list[dict[str, str|dict[str, str]]]

class Scheme(Protocol):
    @classmethod
    def spec(cls) -> SchemeSpec:
        ...