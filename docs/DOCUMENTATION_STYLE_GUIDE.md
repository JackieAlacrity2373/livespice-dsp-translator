# Documentation Style Guide

This guide outlines the conventions and best practices for writing documentation in the LiveSpice DSP Translator project.

## General Guidelines

### Clarity and Conciseness
- Write in clear, concise language
- Use active voice when possible
- Keep sentences and paragraphs short and focused
- Use technical terms accurately and consistently

### Structure
- Use hierarchical headings (H1, H2, H3, etc.) to organize content
- Start with the most important information
- Include a table of contents for longer documents
- Use bullet points and numbered lists for easy scanning

### Formatting
- Use code blocks for code snippets with appropriate syntax highlighting
- Use inline code formatting for file names, commands, and short code references
- Use bold for emphasis sparingly
- Use tables to present structured data clearly

### Language and Tone
- Write for a technical audience
- Be professional and precise
- Avoid colloquialisms and informal language
- Use inclusive language

## Specific Conventions

### Emojis
**Emojis are strongly discouraged in all project documentation.**

Reasons:
- They can render differently across platforms and devices
- They may not be accessible to screen readers
- They add visual clutter without semantic value
- They can appear unprofessional in technical documentation
- They may not display correctly in all text editors and terminals

Instead of emojis, use:
- Plain text symbols (e.g., `[x]` for completed items, `[ ]` for pending)
- Descriptive text (e.g., "Status: Complete" instead of a checkmark emoji)
- Markdown formatting for emphasis
- Semantic HTML comments when appropriate

### Code Examples
- Always test code examples before including them in documentation
- Include necessary context (imports, setup, etc.)
- Use syntax highlighting with language-specific code blocks
- Comment complex code appropriately

### Links
- Use descriptive link text (avoid "click here")
- Verify all links are functional
- Use relative paths for internal documentation links
- Clearly indicate when linking to external resources

### File Naming
- Use uppercase for documentation files (e.g., README.md, QUICK_START.md)
- Use underscores to separate words (e.g., STYLE_GUIDE.md)
- Use descriptive, self-explanatory names

### Status and Version Information
- Clearly indicate document status (Draft, Complete, Archived)
- Include last updated date when relevant
- Note version numbers for versioned content

## Document Types

### README Files
- Provide an overview of the directory or module
- Include quick start instructions when applicable
- Link to more detailed documentation
- Keep focused and concise

### Guides and Tutorials
- Start with prerequisites
- Use step-by-step instructions
- Include expected outcomes
- Provide troubleshooting tips

### Reference Documentation
- Be comprehensive and accurate
- Use consistent terminology
- Include all parameters, return values, and exceptions
- Provide usage examples

### Architecture Documents
- Use diagrams where appropriate
- Explain design decisions and rationale
- Document trade-offs
- Keep updated as the system evolves

## Review Process

Before submitting documentation:
1. Check for spelling and grammar errors
2. Verify all links work
3. Test all code examples
4. Ensure formatting renders correctly
5. Confirm adherence to this style guide
6. Have technical accuracy reviewed by a peer

## Maintaining Documentation

- Update documentation when code changes
- Archive obsolete documentation rather than deleting it
- Keep a changelog for major documentation updates
- Regularly review and refresh existing documentation

---

**Last Updated:** 2026-02-01  
**Version:** 1.0
