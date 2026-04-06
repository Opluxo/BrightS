# Kilo Agents Configuration

This file contains custom agent definitions for the BrightS project.

## Available Agents

Add your custom agents here following the format:

```json
{
  "name": "agent-name",
  "description": "What this agent does",
  "tools": ["read", "write", "edit", "bash", ...],
  "env": {
    "KEY": "value"
  }
}
```

## Project-Specific Agents

Consider adding agents for:
- Kernel development
- Filesystem operations  
- Networking components
- Shell command implementation
- Testing utilities
