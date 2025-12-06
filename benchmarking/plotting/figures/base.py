"""
Base figure class and utilities for thesis plots.
"""

import matplotlib.pyplot as plt
from pathlib import Path
from abc import ABC, abstractmethod
from typing import Optional, Tuple
import logging

from ..config import setup_style, FIGURE_SIZES, PLOT_PARAMS

logger = logging.getLogger(__name__)

# ============================================================================
# BASE FIGURE CLASS
# ============================================================================

class BaseFigure(ABC):
    """Abstract base class for all thesis figures."""

    def __init__(
        self,
        figsize: Tuple[float, float] = None,
        title: str = None,
    ):
        """
        Initialize figure.

        Args:
            figsize: Figure size (width, height) in inches
            title: Figure title
        """
        setup_style()
        self.figsize = figsize or FIGURE_SIZES['single']
        self.title = title
        self.fig = None
        self.ax = None

    def create_figure(self, nrows: int = 1, ncols: int = 1) -> Tuple[plt.Figure, plt.Axes]:
        """Create matplotlib figure and axes."""
        self.fig, self.ax = plt.subplots(nrows, ncols, figsize=self.figsize)
        return self.fig, self.ax

    @abstractmethod
    def plot(self, data, **kwargs):
        """Generate the plot. Must be implemented by subclasses."""
        pass

    def finalize(self):
        """Apply final formatting."""
        if self.title and self.ax is not None:
            if hasattr(self.ax, '__iter__'):
                # Multiple axes - use suptitle
                self.fig.suptitle(self.title, fontsize=13, fontweight='bold')
            else:
                self.ax.set_title(self.title)

        plt.tight_layout()

    def save(self, output_path: Path, close: bool = True):
        """Save figure to file."""
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        self.finalize()
        self.fig.savefig(
            output_path,
            dpi=PLOT_PARAMS['save_dpi'],
            bbox_inches='tight',
            facecolor='white'
        )

        if close:
            plt.close(self.fig)

        logger.info(f"Saved: {output_path}")
        return output_path

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

def save_figure(fig: plt.Figure, output_path: Path, close: bool = True) -> Path:
    """Save a matplotlib figure with consistent settings."""
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    fig.savefig(
        output_path,
        dpi=PLOT_PARAMS['save_dpi'],
        bbox_inches='tight',
        facecolor='white'
    )

    if close:
        plt.close(fig)

    return output_path

