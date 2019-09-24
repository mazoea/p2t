# Use pre-commit

```
pip install pre-commit
pre-commit install
```

# Releasing and version change

Use bumpversion

After new patch
```
python scripts/bumpversion patch
```

After new feature
```
python scripts/bumpversion minor
```