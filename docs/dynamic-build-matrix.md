# Dynamic Build Matrix for GitHub Actions

## Overview

The GitHub Actions workflow now automatically derives the build matrix from the `configs/` directory structure, eliminating the need to manually maintain hardcoded board lists.

## How It Works

### Automatic Matrix Generation

When the workflow runs (push/PR events or manual trigger without specific board selection), it:

1. **Scans the configs directory**: Uses `ls -d configs/*/` to find all board configuration directories
2. **Generates JSON array**: Converts the directory names into a JSON array format
3. **Feeds to build matrix**: The build job uses this dynamic list as its strategy matrix

### Manual Board Selection

When using `workflow_dispatch` (manual trigger):
- **Single board selected**: Builds only the specified board
- **No board selected** (empty option): Builds all boards from configs/ directory

## Benefits

✅ **Automatic synchronization**: Adding/removing config directories automatically updates the build matrix  
✅ **No maintenance**: No need to manually update hardcoded lists in the workflow  
✅ **Always current**: Matrix reflects the actual directory structure  
✅ **Prevents errors**: Can't accidentally reference non-existent configurations  

## Workflow Structure

```mermaid
graph TD
    A[Trigger: Push/PR/Manual] --> B[determine-matrix job]
    B --> C{Manual trigger with board?}
    C -->|Yes| D[Single board matrix]
    C -->|No| E[Scan configs/ directory]
    E --> F[Generate full board matrix]
    D --> G[build job]
    F --> G[build job]
    G --> H[Build selected board(s)]
```

## Implementation Details

### Matrix Generation Script

```bash
# Get all directories in configs/ (excluding the configs directory itself)
cd configs
BOARD_DIRS=$(ls -d */ 2>/dev/null | sed 's|/||' | sort)
cd ..

# Convert to JSON array
BOARD_JSON="["
FIRST=true
for board in $BOARD_DIRS; do
  if [ "$FIRST" = true ]; then
    BOARD_JSON="$BOARD_JSON\"$board\""
    FIRST=false
  else
    BOARD_JSON="$BOARD_JSON, \"$board\""
  fi
done
BOARD_JSON="$BOARD_JSON]"
```

### Job Dependencies

The workflow now has three jobs:
1. `call-node-workflow`: Builds web assets
2. `determine-matrix`: Scans configs/ and generates build matrix
3. `build`: Uses the generated matrix to build board configurations

## Manual Workflow Options

**Important**: GitHub Actions `workflow_dispatch` options must still be manually maintained since they don't support dynamic generation. The options list should match the configs/ directories.

### Updating Workflow Options

To update the workflow_dispatch options when configs/ directories change:

1. Run the helper script: `scripts/update-workflow-boards.ps1`
2. Copy the generated options list to `.github/workflows/cmake.yml`
3. Replace the existing options section

### Helper Script

```powershell
# scripts/update-workflow-boards.ps1
$boards = Get-ChildItem -Path "configs" -Directory | Select-Object -ExpandProperty Name | Sort-Object
Write-Host "workflow_dispatch options:"
Write-Host "          - `"`""
foreach ($board in $boards) {
    Write-Host "          - $board"
}
```

## Example Usage

### Build All Boards
- **Push to main**: Automatically builds all boards
- **Pull request**: Automatically builds all boards  
- **Manual trigger**: Leave board_config empty

### Build Single Board
- **Manual trigger**: Select specific board from dropdown

### Add New Board
1. Create new directory in `configs/YourBoard/`
2. Add board configuration files
3. **Automatic**: Next workflow run will include the new board
4. **Manual**: Update workflow_dispatch options if needed

## Troubleshooting

### Board Not Building
- Check that the directory exists in `configs/`
- Verify directory name matches exactly (case-sensitive)
- Ensure directory contains required configuration files

### Matrix Generation Issues
- Check the `determine-matrix` job logs for directory scanning output
- Verify the generated JSON array format in workflow logs

### Workflow Dispatch Options Out of Sync
- Run `scripts/update-workflow-boards.ps1` to get current list
- Update `.github/workflows/cmake.yml` with new options

This dynamic approach significantly simplifies maintenance while ensuring the build matrix always reflects the actual available board configurations.
